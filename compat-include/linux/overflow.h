#ifndef _NET_OVPN_LINUX_OVERFLOW_H_
#define _NET_OVPN_LINUX_OVERFLOW_H_

#include <linux/version.h>
#include_next <linux/overflow.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 321)

static inline size_t __must_check ovpn_size_mul(size_t factor1, size_t factor2)
{
	size_t bytes;

	if (check_mul_overflow(factor1, factor2, &bytes))
		return SIZE_MAX;

	return bytes;
}

#define size_mul ovpn_size_mul

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 321) */

#endif /* _NET_OVPN_LINUX_OVERFLOW_H_ */
