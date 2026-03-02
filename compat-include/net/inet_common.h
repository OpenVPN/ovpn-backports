#ifndef _NET_OVPN_NET_INET_COMMON_H_
#define _NET_OVPN_NET_INET_COMMON_H_

#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(9, 4)

#include <net/sock.h>
#include_next <net/inet_common.h>

#else

#include_next <net/inet_common.h>

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(9, 4) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 0)

#include <net/busy_poll.h>

static inline struct sk_buff *ovpn_skb_set_peeked(struct sk_buff *skb)
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

static inline struct sk_buff *
ovpn__skb_try_recv_from_queue(struct sk_buff_head *queue, unsigned int flags,
			      int *off, int *err, struct sk_buff **last)
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

static inline struct sk_buff *
ovpn__skb_try_recv_datagram(struct sock *sk, struct sk_buff_head *queue,
			    unsigned int flags, int *off, int *err,
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

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 0)
static inline struct sk_buff *
ovpn__skb_recv_datagram(struct sock *sk, struct sk_buff_head *sk_queue,
			unsigned int flags, int *off, int *err);
#endif

static inline struct sk_buff *
ovpn_skb_recv_datagram(struct sock *sk, struct sk_buff_head *queue,
		       unsigned int flags, int *off, int *err)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0) || \
	RHEL_RELEASE_CODE > RHEL_RELEASE_VERSION(8, 10)
	return __skb_recv_datagram(sk, queue, flags, off, err);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0) || \
	RHEL_RELEASE_CODE > RHEL_RELEASE_VERSION(8, 0)
	return __skb_recv_datagram(sk, queue, flags, NULL, off, err);
#else
	return ovpn__skb_recv_datagram(sk, queue, flags, off, err);
#endif
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 17, 6)

static inline int ovpn_connection_based(struct sock *sk)
{
	return sk->sk_type == SOCK_SEQPACKET || sk->sk_type == SOCK_STREAM;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 0)

static inline int ovpn_receiver_wake_function(wait_queue_entry_t *wait,
					       unsigned int mode, int sync,
					       void *key)
{
	/*
	 * Avoid a wakeup if event not interesting for us
	 */
	if (key && !(key_to_poll(key) & (EPOLLIN | EPOLLERR)))
		return 0;
	return autoremove_wake_function(wait, mode, sync, key);
}

static inline int
ovpn__skb_wait_for_more_packets(struct sock *sk, struct sk_buff_head *queue,
				int *err, long *timeo_p,
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

static inline struct sk_buff *
ovpn__skb_recv_datagram(struct sock *sk, struct sk_buff_head *sk_queue,
			unsigned int flags, int *off, int *err)
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

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 0) */

static inline __poll_t ovpn_datagram_poll_queue(struct file *file,
						 struct socket *sock,
						 poll_table *wait,
						 struct sk_buff_head *rcv_queue)
{
	struct sock *sk = sock->sk;
	__poll_t mask;
	u8 shutdown;

	sock_poll_wait(file, sock, wait);
	mask = 0;

	/* exceptional events? */
	if (READ_ONCE(sk->sk_err) ||
	    !skb_queue_empty_lockless(&sk->sk_error_queue))
		mask |= EPOLLERR |
			(sock_flag(sk, SOCK_SELECT_ERR_QUEUE) ? EPOLLPRI : 0);

	shutdown = READ_ONCE(sk->sk_shutdown);
	if (shutdown & RCV_SHUTDOWN)
		mask |= EPOLLRDHUP | EPOLLIN | EPOLLRDNORM;
	if (shutdown == SHUTDOWN_MASK)
		mask |= EPOLLHUP;

	/* readable? */
	if (!skb_queue_empty_lockless(rcv_queue))
		mask |= EPOLLIN | EPOLLRDNORM;

	/* Connection-based need to check for termination and startup. */
	if (ovpn_connection_based(sk)) {
		int state = READ_ONCE(sk->sk_state);

		if (state == TCP_CLOSE)
			mask |= EPOLLHUP;
		/* connection hasn't started yet? */
		if (state == TCP_SYN_SENT)
			return mask;
	}

	/* writable? */
	if (sock_writeable(sk))
		mask |= EPOLLOUT | EPOLLWRNORM | EPOLLWRBAND;
	else
		sk_set_bit(SOCKWQ_ASYNC_NOSPACE, sk);

	return mask;
}

#define datagram_poll_queue ovpn_datagram_poll_queue

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 17, 6) */

#endif /* _NET_OVPN_NET_INET_COMMON_H_ */
