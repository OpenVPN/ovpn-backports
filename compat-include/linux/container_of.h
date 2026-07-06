#ifndef _NET_OVPN_LINUX_CONTAINER_OF_H_
#define _NET_OVPN_LINUX_CONTAINER_OF_H_

#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
#include_next <linux/container_of.h>
#else
#include <linux/kernel.h>
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0) */

#ifndef container_of_const
#define container_of_const(ptr, type, member)				\
	_Generic(ptr,							\
		const typeof(*(ptr)) *: ((const type *)container_of(ptr, type, member)), \
		default: ((type *)container_of(ptr, type, member))	\
	)
#endif /* container_of_const */

#endif /* _NET_OVPN_LINUX_CONTAINER_OF_H_ */
