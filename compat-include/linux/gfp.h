#ifndef _NET_OVPN_LINUX_GFP_H_
#define _NET_OVPN_LINUX_GFP_H_

#include_next <linux/gfp.h>

#ifndef __default_gfp
#define __default_gfp(a,...) a
#endif /* __default_gfp */

#ifndef default_gfp
#define default_gfp(...) __default_gfp(__VA_ARGS__ __VA_OPT__(,) GFP_KERNEL)
#endif /* default_gfp */

#endif /* _NET_OVPN_LINUX_GFP_H_ */
