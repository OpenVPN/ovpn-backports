#ifndef _NET_OVPN_NET_IP6_FIB_H_
#define _NET_OVPN_NET_IP6_FIB_H_

#include <linux/version.h>
#include_next <net/ip6_fib.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 10, 0)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
#include <linux/container_of.h>
#else
#include <linux/kernel.h>
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0) */

#ifndef container_of_const
#define ovpn_container_of_const(ptr, type, member)			\
	_Generic(ptr,							\
		const typeof(*(ptr)) *: ((const type *)container_of(ptr, type, member)), \
		default: ((type *)container_of(ptr, type, member))	\
	)
#else
#define ovpn_container_of_const(ptr, type, member) \
	container_of_const(ptr, type, member)
#endif /* container_of_const */

#ifndef dst_rt6_info
#define dst_rt6_info(_ptr) ovpn_container_of_const(_ptr, struct rt6_info, dst)
#endif /* dst_rt6_info */

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 10, 0) */

#endif /* _NET_OVPN_NET_IP6_FIB_H_ */
