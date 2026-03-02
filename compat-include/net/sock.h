#ifndef _NET_OVPN_NET_SOCK_H_
#define _NET_OVPN_NET_SOCK_H_

#include <linux/version.h>
#include_next <net/sock.h>

static inline void ovpn_sock_disable_task_frag(struct sock *sk)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0) || \
	RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9, 3)
	sk->sk_use_task_frag = false;
#endif
}

#endif /* _NET_OVPN_NET_SOCK_H_ */
