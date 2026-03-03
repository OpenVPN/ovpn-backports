#ifndef _NET_OVPN_LINUX_WORKQUEUE_H_
#define _NET_OVPN_LINUX_WORKQUEUE_H_

#include <linux/version.h>
#include_next <linux/workqueue.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 17, 0)

#define system_percpu_wq system_wq

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 17, 0) */

#endif /* _NET_OVPN_LINUX_WORKQUEUE_H_ */
