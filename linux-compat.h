#ifndef _NET_OVPN_LINUX_COMPAT_H_
#define _NET_OVPN_LINUX_COMPAT_H_

#include <linux/kconfig.h>
#include <linux/version.h>

/*
 *  Red Hat Enterprise Linux and SUSE Linux Enterprise kernels provide
 *  helper macros for detecting the distribution version.  This is needed
 *  here as Red Hat and SUSE backport features and changes from newer kernels
 *  into the older kernel baseline.  Therefore the RHEL and SLE kernel
 *  features may not be correctly identified by the Linux kernel
 *  version alone.
 *
 *  To be able to build ovpn-dco on non-RHEL/SLE kernels, we need
 *  these helper macros defined.  And we want the result to
 *  always be true, to not disable the other kernel version
 *  checks
 */
#ifndef RHEL_RELEASE_CODE
#define RHEL_RELEASE_CODE 0
#endif
#ifndef RHEL_RELEASE_VERSION
#define RHEL_RELEASE_VERSION(m, n) 1
#endif

#ifndef SUSE_PRODUCT_CODE
#define SUSE_PRODUCT_CODE 0
#endif
#ifndef SUSE_PRODUCT
#define SUSE_PRODUCT(pr, v, pl, aux) 1
#endif

#include <linux/if_link.h>

#ifndef UDP_ENCAP_OVPNINUDP
/* Our UDP encapsulation types, must be unique
 * (other values in include/uapi/linux/udp.h)
 */
#define UDP_ENCAP_OVPNINUDP 8 /* transport layer */
#endif /* UDP_ENCAP_OVPNINUDP */

#ifndef IFLA_OVPN_MAX

enum ovpn_mode {
	OVPN_MODE_P2P,
	OVPN_MODE_MP,
};

enum {
	IFLA_OVPN_UNSPEC,
	IFLA_OVPN_MODE,
	__IFLA_OVPN_MAX,
};

#define IFLA_OVPN_MAX (__IFLA_OVPN_MAX - 1)

#endif /* IFLA_OVPN_MAX */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 5, 0) && \
	LINUX_VERSION_CODE < KERNEL_VERSION(6, 16, 0)

#include <net/sock.h>
static int ovpn_sendmsg_locked(struct sock *sk, struct msghdr *msg)
{
	struct socket *sock = sk->sk_socket;
	size_t size = msg_data_left(msg);

	if (!sock)
		return -EINVAL;

	if (!sock->ops->sendmsg_locked)
		return -EOPNOTSUPP;

	return sock->ops->sendmsg_locked(sk, msg, size);
}

#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(6, 5, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(6, 16, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0)

/**
 * include/net/net_debug.h was introduced in v5.19
 */

#if defined(CONFIG_DEBUG_NET)
#define DEBUG_NET_WARN_ON_ONCE(cond) ((void)WARN_ON_ONCE(cond))
#define DEBUG_NET_WARN_ONCE(cond, format...) ((void)WARN_ONCE(cond, format))
#else
#define DEBUG_NET_WARN_ON_ONCE(cond) BUILD_BUG_ON_INVALID(cond)
#define DEBUG_NET_WARN_ONCE(cond, format...) BUILD_BUG_ON_INVALID(cond)
#endif

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0) && \
	SUSE_PRODUCT_CODE < SUSE_PRODUCT(1, 15, 5, 0)

#define sock_is_readable stream_memory_read

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0) && SUSE_PRODUCT_CODE < SUSE_PRODUCT(1, 15, 5, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 0)

#include <net/ip_tunnels.h>

#define dev_get_tstats64 ip_tunnel_get_stats64

#include <linux/netdevice.h>

static inline void dev_sw_netstats_tx_add(struct net_device *dev,
					  unsigned int packets,
					  unsigned int len)
{
	struct pcpu_sw_netstats *tstats = this_cpu_ptr(dev->tstats);

	u64_stats_update_begin(&tstats->syncp);
	tstats->tx_bytes += len;
	tstats->tx_packets += packets;
	u64_stats_update_end(&tstats->syncp);
}

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 0)

#include <linux/netdevice.h>

static inline void dev_sw_netstats_rx_add(struct net_device *dev,
					  unsigned int len)
{
	struct pcpu_sw_netstats *tstats = this_cpu_ptr(dev->tstats);

	u64_stats_update_begin(&tstats->syncp);
	tstats->rx_bytes += len;
	tstats->rx_packets++;
	u64_stats_update_end(&tstats->syncp);
}

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)

/* Iterate through singly-linked GSO fragments of an skb. */
#define skb_list_walk_safe(first, skb, next_skb)                              \
	for ((skb) = (first), (next_skb) = (skb) ? (skb)->next : NULL; (skb); \
	     (skb) = (next_skb), (next_skb) = (skb) ? (skb)->next : NULL)

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
/**
 * rcu_replace_pointer() - replace an RCU pointer, returning its old value
 * @rcu_ptr: RCU pointer, whose old value is returned
 * @ptr: regular pointer
 * @c: the lockdep conditions under which the dereference will take place
 *
 * Perform a replacement, where @rcu_ptr is an RCU-annotated
 * pointer and @c is the lockdep argument that is passed to the
 * rcu_dereference_protected() call used to read that pointer.  The old
 * value of @rcu_ptr is returned, and @rcu_ptr is set to @ptr.
 */
#undef rcu_replace_pointer
#define rcu_replace_pointer(rcu_ptr, ptr, c)                                   \
	({                                                                     \
		typeof(ptr) __tmp = rcu_dereference_protected((rcu_ptr), (c)); \
		rcu_assign_pointer((rcu_ptr), (ptr));                          \
		__tmp;                                                         \
	})

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0) && \
	SUSE_PRODUCT_CODE < SUSE_PRODUCT(1, 15, 3, 0)

/* commit 895b5c9f206e renamed nf_reset to nf_reset_ct */
#undef nf_reset_ct
#define nf_reset_ct nf_reset

#ifndef fallthrough
#define fallthrough (0)
#endif

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0) && SUSE_PRODUCT_CODE < SUSE_PRODUCT(1, 15, 3, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0)

/* commit 1550c171935d introduced rt_gw4 and rt_gw6 for IPv6 gateways */
#define rt_gw4 rt_gateway

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 0)

#ifndef atomic64_fetch_add_unless
static inline long long atomic64_fetch_add_unless(atomic64_t *v, long long a,
						  long long u)
{
	long long c = atomic64_read(v);

	do {
		if (unlikely(c == u))
			break;
	} while (!atomic64_try_cmpxchg(v, &c, c + a));

	return c;
}
#endif

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 0) */

#endif /* _NET_OVPN_LINUX_COMPAT_H_ */
