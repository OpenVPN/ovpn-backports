// SPDX-License-Identifier: GPL-2.0
/*  OpenVPN data channel offload
 *
 *  Copyright (C) 2019-2025 OpenVPN, Inc.
 *
 *  Author:	Antonio Quartulli <antonio@openvpn.net>
 */

#include <linux/skbuff.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 0)
#include <net/busy_poll.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
#include <net/hotdata.h>
#endif
#include <net/inet_common.h>
#include <net/ipv6.h>
#include <net/tcp.h>
#include <net/transp_v6.h>
#include <net/route.h>
#include <trace/events/sock.h>

#include "ovpnpriv.h"
#include "main.h"
#include "io.h"
#include "peer.h"
#include "proto.h"
#include "skb.h"
#include "tcp.h"

#define OVPN_TCP_DEPTH_NESTING	2
#if OVPN_TCP_DEPTH_NESTING == SINGLE_DEPTH_NESTING
#error "OVPN TCP requires its own lockdep subclass"
#endif

static struct proto ovpn_tcp_prot __ro_after_init;
static struct proto_ops ovpn_tcp_ops __ro_after_init;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 16, 0)
static struct proto ovpn_tcp6_prot;
static struct proto_ops ovpn_tcp6_ops;
static DEFINE_MUTEX(tcp6_prot_mutex);
#else
static struct proto ovpn_tcp6_prot __ro_after_init;
static struct proto_ops ovpn_tcp6_ops __ro_after_init;
#endif

static int ovpn_tcp_parse(struct strparser *strp, struct sk_buff *skb)
{
	struct strp_msg *rxm = strp_msg(skb);
	__be16 blen;
	u16 len;
	int err;

	/* when packets are written to the TCP stream, they are prepended with
	 * two bytes indicating the actual packet size.
	 * Parse accordingly and return the actual size (including the size
	 * header)
	 */

	if (skb->len < rxm->offset + 2)
		return 0;

	err = skb_copy_bits(skb, rxm->offset, &blen, sizeof(blen));
	if (err < 0)
		return err;

	len = be16_to_cpu(blen);
	if (len < 2)
		return -EINVAL;

	return len + 2;
}

/* queue skb for sending to userspace via recvmsg on the socket */
static void ovpn_tcp_to_userspace(struct ovpn_peer *peer, struct sock *sk,
				  struct sk_buff *skb)
{
	skb_set_owner_r(skb, sk);
	memset(skb->cb, 0, sizeof(skb->cb));
	skb_queue_tail(&peer->tcp.user_queue, skb);
	peer->tcp.sk_cb.sk_data_ready(sk);
}

static void ovpn_tcp_rcv(struct strparser *strp, struct sk_buff *skb)
{
	struct ovpn_peer *peer = container_of(strp, struct ovpn_peer, tcp.strp);
	struct strp_msg *msg = strp_msg(skb);
	size_t pkt_len = msg->full_len - 2;
	size_t off = msg->offset + 2;
	u8 opcode;

	/* ensure skb->data points to the beginning of the openvpn packet */
	if (!pskb_pull(skb, off)) {
		net_warn_ratelimited("%s: packet too small for peer %u\n",
				     netdev_name(peer->ovpn->dev), peer->id);
		goto err;
	}

	/* strparser does not trim the skb for us, therefore we do it now */
	if (pskb_trim(skb, pkt_len) != 0) {
		net_warn_ratelimited("%s: trimming skb failed for peer %u\n",
				     netdev_name(peer->ovpn->dev), peer->id);
		goto err;
	}

	/* we need the first 4 bytes of data to be accessible
	 * to extract the opcode and the key ID later on
	 */
	if (!pskb_may_pull(skb, OVPN_OPCODE_SIZE)) {
		net_warn_ratelimited("%s: packet too small to fetch opcode for peer %u\n",
				     netdev_name(peer->ovpn->dev), peer->id);
		goto err;
	}

	/* DATA_V2 packets are handled in kernel, the rest goes to user space */
	opcode = ovpn_opcode_from_skb(skb, 0);
	if (unlikely(opcode != OVPN_DATA_V2)) {
		if (opcode == OVPN_DATA_V1) {
			net_warn_ratelimited("%s: DATA_V1 detected on the TCP stream\n",
					     netdev_name(peer->ovpn->dev));
			goto err;
		}

		/* The packet size header must be there when sending the packet
		 * to userspace, therefore we put it back
		 */
		skb_push(skb, 2);
		ovpn_tcp_to_userspace(peer, strp->sk, skb);
		return;
	}

	/* hold reference to peer as required by ovpn_recv().
	 *
	 * NOTE: in this context we should already be holding a reference to
	 * this peer, therefore ovpn_peer_hold() is not expected to fail
	 */
	if (WARN_ON(!ovpn_peer_hold(peer)))
		goto err_nopeer;

	ovpn_recv(peer, skb);
	return;
err:
	/* take reference for deferred peer deletion. should never fail */
	if (WARN_ON(!ovpn_peer_hold(peer)))
		goto err_nopeer;
	schedule_work(&peer->tcp.defer_del_work);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 14, 0)
	dev_dstats_rx_dropped(peer->ovpn->dev);
#else
	dev_core_stats_rx_dropped_inc(peer->ovpn->dev);
#endif
err_nopeer:
	kfree_skb(skb);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 0)
static struct sk_buff *ovpn_skb_set_peeked(struct sk_buff *skb)
{
	struct sk_buff *nskb;

	if (skb->peeked)
		return skb;

	/* We have to unshare an skb before modifying it. */
	if (!skb_shared(skb))
		goto done;

	nskb = skb_clone(skb, GFP_ATOMIC);
	if (!nskb)
		return ERR_PTR(-ENOMEM);

	skb->prev->next = nskb;
	skb->next->prev = nskb;
	nskb->prev = skb->prev;
	nskb->next = skb->next;

	consume_skb(skb);
	skb = nskb;

done:
	skb->peeked = 1;

	return skb;
}

static struct sk_buff *ovpn__skb_try_recv_from_queue(struct sk_buff_head *queue,
						     unsigned int flags,
						     int *off, int *err,
						     struct sk_buff **last)
{
	bool peek_at_off = false;
	struct sk_buff *skb;
	int _off = 0;

	if (unlikely(flags & MSG_PEEK && *off >= 0)) {
		peek_at_off = true;
		_off = *off;
	}

	*last = queue->prev;
	skb_queue_walk(queue, skb)
	{
		if (flags & MSG_PEEK) {
			if (peek_at_off && _off >= skb->len &&
			    (_off || skb->peeked)) {
				_off -= skb->len;
				continue;
			}
			if (!skb->len) {
				skb = ovpn_skb_set_peeked(skb);
				if (IS_ERR(skb)) {
					*err = PTR_ERR(skb);
					return NULL;
				}
			}
			refcount_inc(&skb->users);
		} else {
			__skb_unlink(skb, queue);
		}
		*off = _off;
		return skb;
	}
	return NULL;
}

static struct sk_buff *ovpn__skb_try_recv_datagram(struct sock *sk,
						   struct sk_buff_head *queue,
						   unsigned int flags, int *off,
						   int *err,
						   struct sk_buff **last)
{
	struct sk_buff *skb;
	unsigned long cpu_flags;
	/*
	 * Caller is allowed not to check sk->sk_err before skb_recv_datagram()
	 */
	int error = sock_error(sk);

	if (error)
		goto no_packet;

	do {
		/* Again only user level code calls this function, so nothing
		 * interrupt level will suddenly eat the receive_queue.
		 *
		 * Look at current nfs client by the way...
		 * However, this function was correct in any case. 8)
		 */
		spin_lock_irqsave(&queue->lock, cpu_flags);
		skb = ovpn__skb_try_recv_from_queue(queue, flags, off, &error,
						    last);
		spin_unlock_irqrestore(&queue->lock, cpu_flags);
		if (error)
			goto no_packet;
		if (skb)
			return skb;

		if (!sk_can_busy_loop(sk))
			break;

		sk_busy_loop(sk, flags & MSG_DONTWAIT);
	} while (READ_ONCE(queue->prev) != *last);

	error = -EAGAIN;

no_packet:
	*err = error;
	return NULL;
}

static inline int ovpn_connection_based(struct sock *sk)
{
	return sk->sk_type == SOCK_SEQPACKET || sk->sk_type == SOCK_STREAM;
}

static int ovpn_receiver_wake_function(wait_queue_entry_t *wait,
				       unsigned int mode, int sync, void *key)
{
	/*
	 * Avoid a wakeup if event not interesting for us
	 */
	if (key && !(key_to_poll(key) & (EPOLLIN | EPOLLERR)))
		return 0;
	return autoremove_wake_function(wait, mode, sync, key);
}

static int ovpn__skb_wait_for_more_packets(struct sock *sk,
					   struct sk_buff_head *queue, int *err,
					   long *timeo_p,
					   const struct sk_buff *skb)
{
	int error;
	DEFINE_WAIT_FUNC(wait, ovpn_receiver_wake_function);

	prepare_to_wait_exclusive(sk_sleep(sk), &wait, TASK_INTERRUPTIBLE);

	/* Socket errors? */
	error = sock_error(sk);
	if (error)
		goto out_err;

	if (READ_ONCE(queue->prev) != skb)
		goto out;

	/* Socket shut down? */
	if (sk->sk_shutdown & RCV_SHUTDOWN)
		goto out_noerr;

	/* Sequenced packets can come disconnected.
	 * If so we report the problem
	 */
	error = -ENOTCONN;
	if (ovpn_connection_based(sk) &&
	    !(sk->sk_state == TCP_ESTABLISHED || sk->sk_state == TCP_LISTEN))
		goto out_err;

	/* handle signals */
	if (signal_pending(current))
		goto interrupted;

	error = 0;
	*timeo_p = schedule_timeout(*timeo_p);
out:
	finish_wait(sk_sleep(sk), &wait);
	return error;
interrupted:
	error = sock_intr_errno(*timeo_p);
out_err:
	*err = error;
	goto out;
out_noerr:
	*err = 0;
	error = 1;
	goto out;
}

static struct sk_buff *ovpn__skb_recv_datagram(struct sock *sk,
					       struct sk_buff_head *sk_queue,
					       unsigned int flags, int *off,
					       int *err)
{
	struct sk_buff *skb, *last;
	long timeo;

	timeo = sock_rcvtimeo(sk, flags & MSG_DONTWAIT);

	do {
		skb = ovpn__skb_try_recv_datagram(sk, sk_queue, flags, off, err,
						  &last);
		if (skb)
			return skb;

		if (*err != -EAGAIN)
			break;
	} while (timeo && !ovpn__skb_wait_for_more_packets(sk, sk_queue, err,
							   &timeo, last));

	return NULL;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0)
static int ovpn_tcp_recvmsg(struct sock *sk, struct msghdr *msg, size_t len,
			    int _noblock, int flags, int *addr_len)
#else
static int ovpn_tcp_recvmsg(struct sock *sk, struct msghdr *msg, size_t len,
			    int flags, int *addr_len)
#endif
{
	int err = 0, off, copied = 0, ret;
	struct ovpn_socket *sock;
	struct ovpn_peer *peer;
	struct sk_buff *skb;

	rcu_read_lock();
	sock = rcu_dereference_sk_user_data(sk);
	if (unlikely(!sock || !sock->peer || !ovpn_peer_hold(sock->peer))) {
		rcu_read_unlock();
		return -EBADF;
	}
	peer = sock->peer;
	rcu_read_unlock();

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0) || RHEL_RELEASE_CODE > RHEL_RELEASE_VERSION(8, 10)
	skb = __skb_recv_datagram(sk, &peer->tcp.user_queue, flags, &off, &err);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0) || RHEL_RELEASE_CODE > RHEL_RELEASE_VERSION(8, 0)
	skb = __skb_recv_datagram(sk, &peer->tcp.user_queue, flags, NULL, &off,
				  &err);
#else
	skb = ovpn__skb_recv_datagram(sk, &peer->tcp.user_queue, flags, &off,
				      &err);
#endif
	if (!skb) {
		if (err == -EAGAIN && sk->sk_shutdown & RCV_SHUTDOWN) {
			ret = 0;
			goto out;
		}
		ret = err;
		goto out;
	}

	copied = len;
	if (copied > skb->len)
		copied = skb->len;
	else if (copied < skb->len)
		msg->msg_flags |= MSG_TRUNC;

	err = skb_copy_datagram_msg(skb, 0, msg, copied);
	if (unlikely(err)) {
		kfree_skb(skb);
		ret = err;
		goto out;
	}

	if (flags & MSG_TRUNC)
		copied = skb->len;
	kfree_skb(skb);
	ret = copied;
out:
	ovpn_peer_put(peer);
	return ret;
}

void ovpn_tcp_socket_detach(struct ovpn_socket *ovpn_sock)
{
	struct ovpn_peer *peer = ovpn_sock->peer;
	struct sock *sk = ovpn_sock->sk;

	strp_stop(&peer->tcp.strp);
	skb_queue_purge(&peer->tcp.user_queue);

	/* restore CBs that were saved in ovpn_sock_set_tcp_cb() */
	sk->sk_data_ready = peer->tcp.sk_cb.sk_data_ready;
	sk->sk_write_space = peer->tcp.sk_cb.sk_write_space;
	sk->sk_prot = peer->tcp.sk_cb.prot;
	sk->sk_socket->ops = peer->tcp.sk_cb.ops;

	rcu_assign_sk_user_data(sk, NULL);
}

void ovpn_tcp_socket_wait_finish(struct ovpn_socket *sock)
{
	struct ovpn_peer *peer = sock->peer;

	/* NOTE: we don't wait for peer->tcp.defer_del_work to finish:
	 * either the worker is not running or this function
	 * was invoked by that worker.
	 */

	cancel_work_sync(&sock->tcp_tx_work);
	strp_done(&peer->tcp.strp);

	skb_queue_purge(&peer->tcp.out_queue);
	kfree_skb(peer->tcp.out_msg.skb);
	peer->tcp.out_msg.skb = NULL;
}

static void ovpn_tcp_send_sock(struct ovpn_peer *peer, struct sock *sk)
{
	struct sk_buff *skb = peer->tcp.out_msg.skb;
	int ret, flags;

	if (!skb)
		return;

	if (peer->tcp.tx_in_progress)
		return;

	peer->tcp.tx_in_progress = true;

	do {
		flags = ovpn_skb_cb(skb)->nosignal ? MSG_NOSIGNAL : 0;
		ret = skb_send_sock_locked_with_flags(sk, skb,
						      peer->tcp.out_msg.offset,
						      peer->tcp.out_msg.len,
						      flags);
		if (unlikely(ret < 0)) {
			if (ret == -EAGAIN)
				goto out;

			net_warn_ratelimited("%s: TCP error to peer %u: %d\n",
					     netdev_name(peer->ovpn->dev),
					     peer->id, ret);

			/* in case of TCP error we can't recover the VPN
			 * stream therefore we abort the connection
			 */
			ovpn_peer_hold(peer);
			schedule_work(&peer->tcp.defer_del_work);

			/* we bail out immediately and keep tx_in_progress set
			 * to true. This way we prevent more TX attempts
			 * which would lead to more invocations of
			 * schedule_work()
			 */
			return;
		}

		peer->tcp.out_msg.len -= ret;
		peer->tcp.out_msg.offset += ret;
	} while (peer->tcp.out_msg.len > 0);

	if (!peer->tcp.out_msg.len) {
		preempt_disable();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 14, 0)
		dev_dstats_tx_add(peer->ovpn->dev, skb->len);
#else
		dev_sw_netstats_tx_add(peer->ovpn->dev, 1, skb->len);
#endif
		preempt_enable();
	}

	kfree_skb(peer->tcp.out_msg.skb);
	peer->tcp.out_msg.skb = NULL;
	peer->tcp.out_msg.len = 0;
	peer->tcp.out_msg.offset = 0;

out:
	peer->tcp.tx_in_progress = false;
}

void ovpn_tcp_tx_work(struct work_struct *work)
{
	struct ovpn_socket *sock;

	sock = container_of(work, struct ovpn_socket, tcp_tx_work);

	lock_sock(sock->sk);
	if (sock->peer)
		ovpn_tcp_send_sock(sock->peer, sock->sk);
	release_sock(sock->sk);
}

static void ovpn_tcp_send_sock_skb(struct ovpn_peer *peer, struct sock *sk,
				   struct sk_buff *skb)
{
	if (peer->tcp.out_msg.skb)
		ovpn_tcp_send_sock(peer, sk);

	if (peer->tcp.out_msg.skb) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 14, 0)
		dev_dstats_tx_dropped(peer->ovpn->dev);
#else
		dev_core_stats_tx_dropped_inc(peer->ovpn->dev);
#endif
		kfree_skb(skb);
		return;
	}

	peer->tcp.out_msg.skb = skb;
	peer->tcp.out_msg.len = skb->len;
	peer->tcp.out_msg.offset = 0;
	ovpn_tcp_send_sock(peer, sk);
}

void ovpn_tcp_send_skb(struct ovpn_peer *peer, struct sock *sk,
		       struct sk_buff *skb)
{
	u16 len = skb->len;

	*(__be16 *)__skb_push(skb, sizeof(u16)) = htons(len);

	spin_lock_nested(&sk->sk_lock.slock, OVPN_TCP_DEPTH_NESTING);
	if (sock_owned_by_user(sk)) {
		if (skb_queue_len(&peer->tcp.out_queue) >=
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
		    READ_ONCE(net_hotdata.max_backlog)) {
#else
		    netdev_max_backlog) {
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 14, 0)
			dev_dstats_tx_dropped(peer->ovpn->dev);
#else
			dev_core_stats_tx_dropped_inc(peer->ovpn->dev);
#endif
			kfree_skb(skb);
			goto unlock;
		}
		__skb_queue_tail(&peer->tcp.out_queue, skb);
	} else {
		ovpn_tcp_send_sock_skb(peer, sk, skb);
	}
unlock:
	spin_unlock(&sk->sk_lock.slock);
}

static void ovpn_tcp_release(struct sock *sk)
{
	struct sk_buff_head queue;
	struct ovpn_socket *sock;
	struct ovpn_peer *peer;
	struct sk_buff *skb;

	rcu_read_lock();
	sock = rcu_dereference_sk_user_data(sk);
	if (!sock) {
		rcu_read_unlock();
		return;
	}

	peer = sock->peer;

	/* during initialization this function is called before
	 * assigning sock->peer
	 */
	if (unlikely(!peer || !ovpn_peer_hold(peer))) {
		rcu_read_unlock();
		return;
	}
	rcu_read_unlock();

	__skb_queue_head_init(&queue);
	skb_queue_splice_init(&peer->tcp.out_queue, &queue);

	while ((skb = __skb_dequeue(&queue)))
		ovpn_tcp_send_sock_skb(peer, sk, skb);

	peer->tcp.sk_cb.prot->release_cb(sk);
	ovpn_peer_put(peer);
}

static int ovpn_tcp_sendmsg(struct sock *sk, struct msghdr *msg, size_t size)
{
	struct ovpn_socket *sock;
	int ret, linear = PAGE_SIZE;
	struct ovpn_peer *peer;
	struct sk_buff *skb;

	lock_sock(sk);
	rcu_read_lock();
	sock = rcu_dereference_sk_user_data(sk);
	if (unlikely(!sock || !sock->peer || !ovpn_peer_hold(sock->peer))) {
		rcu_read_unlock();
		release_sock(sk);
		return -EIO;
	}
	rcu_read_unlock();
	peer = sock->peer;

	if (msg->msg_flags & ~(MSG_DONTWAIT | MSG_NOSIGNAL)) {
		ret = -EOPNOTSUPP;
		goto peer_free;
	}

	if (peer->tcp.out_msg.skb) {
		ret = -EAGAIN;
		goto peer_free;
	}

	if (size < linear)
		linear = size;

	skb = sock_alloc_send_pskb(sk, linear, size - linear,
				   msg->msg_flags & MSG_DONTWAIT, &ret, 0);
	if (!skb) {
		net_err_ratelimited("%s: skb alloc failed: %d\n",
				    netdev_name(peer->ovpn->dev), ret);
		goto peer_free;
	}

	skb_put(skb, linear);
	skb->len = size;
	skb->data_len = size - linear;

	ret = skb_copy_datagram_from_iter(skb, 0, &msg->msg_iter, size);
	if (ret) {
		kfree_skb(skb);
		net_err_ratelimited("%s: skb copy from iter failed: %d\n",
				    netdev_name(peer->ovpn->dev), ret);
		goto peer_free;
	}

	ovpn_skb_cb(skb)->nosignal = msg->msg_flags & MSG_NOSIGNAL;
	ovpn_tcp_send_sock_skb(peer, sk, skb);
	ret = size;
peer_free:
	release_sock(sk);
	ovpn_peer_put(peer);
	return ret;
}

static int ovpn_tcp_disconnect(struct sock *sk, int flags)
{
	return -EBUSY;
}

static void ovpn_tcp_data_ready(struct sock *sk)
{
	struct ovpn_socket *sock;

	trace_sk_data_ready(sk);

	rcu_read_lock();
	sock = rcu_dereference_sk_user_data(sk);
	if (likely(sock && sock->peer))
		strp_data_ready(&sock->peer->tcp.strp);
	rcu_read_unlock();
}

static void ovpn_tcp_write_space(struct sock *sk)
{
	struct ovpn_socket *sock;

	rcu_read_lock();
	sock = rcu_dereference_sk_user_data(sk);
	if (likely(sock && sock->peer)) {
		schedule_work(&sock->tcp_tx_work);
		sock->peer->tcp.sk_cb.sk_write_space(sk);
	}
	rcu_read_unlock();
}

static void ovpn_tcp_build_protos(struct proto *new_prot,
				  struct proto_ops *new_ops,
				  const struct proto *orig_prot,
				  const struct proto_ops *orig_ops);

static void ovpn_tcp_peer_del_work(struct work_struct *work)
{
	struct ovpn_peer *peer = container_of(work, struct ovpn_peer,
					      tcp.defer_del_work);

	ovpn_peer_del(peer, OVPN_DEL_PEER_REASON_TRANSPORT_ERROR);
	ovpn_peer_put(peer);
}

/* Set TCP encapsulation callbacks */
int ovpn_tcp_socket_attach(struct ovpn_socket *ovpn_sock,
			   struct ovpn_peer *peer)
{
	struct strp_callbacks cb = {
		.rcv_msg = ovpn_tcp_rcv,
		.parse_msg = ovpn_tcp_parse,
	};
	int ret;

	/* make sure no pre-existing encapsulation handler exists */
	if (ovpn_sock->sk->sk_user_data)
		return -EBUSY;

	/* only a fully connected socket is expected. Connection should be
	 * handled in userspace
	 */
	if (ovpn_sock->sk->sk_state != TCP_ESTABLISHED) {
		net_err_ratelimited("%s: provided TCP socket is not in ESTABLISHED state: %d\n",
				    netdev_name(peer->ovpn->dev),
				    ovpn_sock->sk->sk_state);
		return -EINVAL;
	}

	ret = strp_init(&peer->tcp.strp, ovpn_sock->sk, &cb);
	if (ret < 0) {
		DEBUG_NET_WARN_ON_ONCE(1);
		return ret;
	}

	INIT_WORK(&peer->tcp.defer_del_work, ovpn_tcp_peer_del_work);

	__sk_dst_reset(ovpn_sock->sk);
	skb_queue_head_init(&peer->tcp.user_queue);
	skb_queue_head_init(&peer->tcp.out_queue);

	/* save current CBs so that they can be restored upon socket release */
	peer->tcp.sk_cb.sk_data_ready = ovpn_sock->sk->sk_data_ready;
	peer->tcp.sk_cb.sk_write_space = ovpn_sock->sk->sk_write_space;
	peer->tcp.sk_cb.prot = ovpn_sock->sk->sk_prot;
	peer->tcp.sk_cb.ops = ovpn_sock->sk->sk_socket->ops;

	/* assign our static CBs and prot/ops */
	ovpn_sock->sk->sk_data_ready = ovpn_tcp_data_ready;
	ovpn_sock->sk->sk_write_space = ovpn_tcp_write_space;

	if (ovpn_sock->sk->sk_family == AF_INET) {
		ovpn_sock->sk->sk_prot = &ovpn_tcp_prot;
		ovpn_sock->sk->sk_socket->ops = &ovpn_tcp_ops;
	} else {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 16, 0)
		mutex_lock(&tcp6_prot_mutex);
		if (!ovpn_tcp6_prot.recvmsg)
			ovpn_tcp_build_protos(&ovpn_tcp6_prot, &ovpn_tcp6_ops,
					      ovpn_sock->sk->sk_prot,
					      ovpn_sock->sk->sk_socket->ops);
		mutex_unlock(&tcp6_prot_mutex);
#endif
		ovpn_sock->sk->sk_prot = &ovpn_tcp6_prot;
		ovpn_sock->sk->sk_socket->ops = &ovpn_tcp6_ops;
	}

	/* avoid using task_frag */
	ovpn_sock->sk->sk_allocation = GFP_ATOMIC;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0) || RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9, 3)
	ovpn_sock->sk->sk_use_task_frag = false;
#endif

	/* enqueue the RX worker */
	strp_check_rcv(&peer->tcp.strp);

	return 0;
}

static void ovpn_tcp_close(struct sock *sk, long timeout)
{
	struct ovpn_socket *sock;
	struct ovpn_peer *peer;

	rcu_read_lock();
	sock = rcu_dereference_sk_user_data(sk);
	if (!sock || !sock->peer || !ovpn_peer_hold(sock->peer)) {
		rcu_read_unlock();
		return;
	}
	peer = sock->peer;
	rcu_read_unlock();

	ovpn_peer_del(sock->peer, OVPN_DEL_PEER_REASON_TRANSPORT_DISCONNECT);
	peer->tcp.sk_cb.prot->close(sk, timeout);
	ovpn_peer_put(peer);
}

static __poll_t ovpn_tcp_poll(struct file *file, struct socket *sock,
			      poll_table *wait)
{
	__poll_t mask = datagram_poll(file, sock, wait);
	struct ovpn_socket *ovpn_sock;

	rcu_read_lock();
	ovpn_sock = rcu_dereference_sk_user_data(sock->sk);
	if (ovpn_sock && ovpn_sock->peer &&
	    !skb_queue_empty(&ovpn_sock->peer->tcp.user_queue))
		mask |= EPOLLIN | EPOLLRDNORM;
	rcu_read_unlock();

	return mask;
}

static void ovpn_tcp_build_protos(struct proto *new_prot,
				  struct proto_ops *new_ops,
				  const struct proto *orig_prot,
				  const struct proto_ops *orig_ops)
{
	memcpy(new_prot, orig_prot, sizeof(*new_prot));
	memcpy(new_ops, orig_ops, sizeof(*new_ops));
	new_prot->recvmsg = ovpn_tcp_recvmsg;
	new_prot->sendmsg = ovpn_tcp_sendmsg;
	new_prot->disconnect = ovpn_tcp_disconnect;
	new_prot->close = ovpn_tcp_close;
	new_prot->release_cb = ovpn_tcp_release;
	new_ops->poll = ovpn_tcp_poll;
}

/* Initialize TCP static objects */
void __init ovpn_tcp_init(void)
{
	ovpn_tcp_build_protos(&ovpn_tcp_prot, &ovpn_tcp_ops, &tcp_prot,
			      &inet_stream_ops);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 16, 0)
#if IS_ENABLED(CONFIG_IPV6)
	ovpn_tcp_build_protos(&ovpn_tcp6_prot, &ovpn_tcp6_ops, &tcpv6_prot,
			      &inet6_stream_ops);
#endif
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(6, 16, 0) */
}
