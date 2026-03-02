#ifndef _NET_OVPN_NET_NET_NAMESPACE_H_
#define _NET_OVPN_NET_NET_NAMESPACE_H_

#include <linux/version.h>
#include_next <net/net_namespace.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 83) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 10)

static inline int ovpn_peernet2id_alloc(struct net *net, struct net *peer,
					gfp_t _gfp)
{
	return peernet2id_alloc(net, peer);
}

#define peernet2id_alloc ovpn_peernet2id_alloc

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 83) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 10) */

#endif /* _NET_OVPN_NET_NET_NAMESPACE_H_ */
