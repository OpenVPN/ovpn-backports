#ifndef _NET_OVPN_NET_HOTDATA_H_
#define _NET_OVPN_NET_HOTDATA_H_

#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)

#include_next <net/hotdata.h>

static inline unsigned int ovpn_tcp_max_backlog(void)
{
	return READ_ONCE(net_hotdata.max_backlog);
}

#else

#include <linux/netdevice.h>

static inline unsigned int ovpn_tcp_max_backlog(void)
{
	return READ_ONCE(netdev_max_backlog);
}

#endif

#endif /* _NET_OVPN_NET_HOTDATA_H_ */
