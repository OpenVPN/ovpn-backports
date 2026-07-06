#ifndef _NET_OVPN_NET_IP6_FIB_H_
#define _NET_OVPN_NET_IP6_FIB_H_

#include <linux/version.h>
#include_next <net/ip6_fib.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 10, 0)

#include <linux/container_of.h>

#define dst_rt6_info(_ptr) container_of_const(_ptr, struct rt6_info, dst)

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 10, 0) */

#endif /* _NET_OVPN_NET_IP6_FIB_H_ */
