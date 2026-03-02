#ifndef _NET_OVPN_NET_IP6_ROUTE_H_
#define _NET_OVPN_NET_IP6_ROUTE_H_

#include <linux/version.h>
#include_next <net/ip6_route.h>

static inline bool ovpn_rt4_uses_gateway(const struct rtable *rt)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0) && \
	LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 0)
	return rt->rt_gw_family == AF_INET;
#else
	return rt->rt_uses_gateway;
#endif
}

static inline struct dst_entry *
ovpn_ipv6_dst_lookup_flow(struct net *net, const struct sock *sk,
			  struct flowi6 *fl6,
			  const struct in6_addr *final_dst)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 119) || RHEL_RELEASE_CODE != 0
	return ipv6_stub->ipv6_dst_lookup_flow(net, sk, fl6, final_dst);
#else
	struct dst_entry *entry;
	int err;

	(void)final_dst;

	err = ipv6_stub->ipv6_dst_lookup(net, sk, &entry, fl6);
	if (err)
		return ERR_PTR(err);
	return entry;
#endif
}

#endif /* _NET_OVPN_NET_IP6_ROUTE_H_ */
