#ifndef _NET_OVPN_LINUX_NETDEVICE_H_
#define _NET_OVPN_LINUX_NETDEVICE_H_

#include <linux/version.h>
#include_next <linux/netdevice.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(9, 1)

typedef struct {} netdevice_tracker;
#define __netdev_tracker_alloc(dev, tracker, gfp) do {} while (0)
#define netdev_tracker_free(dev, tracker) do {} while (0)

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(9, 1) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(9, 1)

#define __dev_hold dev_hold
#define __dev_put dev_put

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
#define __netdev_tracker_alloc netdev_tracker_alloc
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0) */

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(9, 1) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0) && \
		RHEL_RELEASE_CODE == 0) || \
		(RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(9, 2) && \
		RHEL_RELEASE_CODE > RHEL_RELEASE_VERSION(8, 10))

#define dev_core_stats_tx_dropped_inc(dev) atomic_long_inc(&dev->tx_dropped)
#define dev_core_stats_rx_dropped_inc(dev) atomic_long_inc(&dev->rx_dropped)

#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0) && RHEL_RELEASE_CODE == 0) || (RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(9, 2) && RHEL_RELEASE_CODE > RHEL_RELEASE_VERSION(8, 10)) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 14, 0)

#define dev_dstats_rx_add(dev, len) dev_sw_netstats_rx_add(dev, len)
#define dev_dstats_tx_add(dev, len) dev_sw_netstats_tx_add(dev, 1, len)
#define dev_dstats_tx_dropped(dev) dev_core_stats_tx_dropped_inc(dev)
#define dev_dstats_rx_dropped(dev) dev_core_stats_rx_dropped_inc(dev)

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 14, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0) && \
	SUSE_PRODUCT_CODE < SUSE_PRODUCT(1, 15, 5, 0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 10)

/**
 * commit 58caed3dacb4 renamed to netif_napi_add_tx_weight,
 * commit c3f760ef1287 removed netif_tx_napi_add
 */
#define netif_napi_add_tx_weight netif_tx_napi_add

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0) && SUSE_PRODUCT_CODE < SUSE_PRODUCT(1, 15, 5, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 10) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(9, 3)

static inline void netdev_hold(struct net_device *dev,
			       netdevice_tracker *tracker, gfp_t gfp)
{
	if (dev) {
		__dev_hold(dev);
		__netdev_tracker_alloc(dev, tracker, gfp);
	}
}

static inline void netdev_put(struct net_device *dev,
			      netdevice_tracker *tracker)
{
	if (dev) {
		netdev_tracker_free(dev, tracker);
		__dev_put(dev);
	}
}

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(9, 3) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0)

#if RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 10)
#include <linux/netdev_features.h>
#undef NETIF_F_SG
#define NETIF_F_SG (__NETIF_F(SG) | NETIF_F_LLTX)
#endif /* RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 10) */

#define lltx needs_free_netdev

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0) */

#endif /* _NET_OVPN_LINUX_NETDEVICE_H_ */
