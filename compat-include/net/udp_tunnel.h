#ifndef _NET_OVPN_NET_UDP_TUNNEL_H_
#define _NET_OVPN_NET_UDP_TUNNEL_H_

#include <linux/version.h>
#include <net/inet_sock.h>
#include <net/udp.h>
#include_next <net/udp_tunnel.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 189) && RHEL_RELEASE_CODE == 0
static inline void
ovpn_setup_udp_tunnel_sock(struct net *net, struct socket *sock,
			   struct udp_tunnel_sock_cfg *cfg)
{
	setup_udp_tunnel_sock(net, sock, cfg);

	/* Pre-v5.4 only enables the IPv6 encap key for PF_INET6 sockets.
	 * Ensure IPv4 receive path is also encap-enabled for dual-stack sockets.
	 */
	if (sock->sk->sk_family == PF_INET6)
		udp_encap_enable();
}

#define setup_udp_tunnel_sock ovpn_setup_udp_tunnel_sock
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 189) && RHEL_RELEASE_CODE == 0 */

static inline void
ovpn_udp_tunnel_xmit_skb(struct rtable *rt, struct sock *sk,
			 struct sk_buff *skb, __be32 src, __be32 dst,
			 __u8 tos, __u8 ttl, __be16 df, __be16 src_port,
			 __be16 dst_port, bool xnet, bool nocheck)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 17, 0)
	udp_tunnel_xmit_skb(rt, sk, skb, src, dst, tos, ttl, df, src_port,
			    dst_port, xnet, nocheck, 0);
#else
	udp_tunnel_xmit_skb(rt, sk, skb, src, dst, tos, ttl, df, src_port,
			    dst_port, xnet, nocheck);
#endif
}

static inline void
ovpn_udp_tunnel6_xmit_skb(struct dst_entry *dst, struct sock *sk,
			  struct sk_buff *skb, struct net_device *dev,
			  struct in6_addr *saddr, struct in6_addr *daddr,
			  __u8 prio, __u8 ttl, __be32 label, __be16 src_port,
			  __be16 dst_port, bool nocheck)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 17, 0)
	udp_tunnel6_xmit_skb(dst, sk, skb, dev, saddr, daddr, prio, ttl, label,
			     src_port, dst_port, nocheck, 0);
#else
	udp_tunnel6_xmit_skb(dst, sk, skb, dev, saddr, daddr, prio, ttl, label,
			     src_port, dst_port, nocheck);
#endif
}

static inline void ovpn_udp_set_mc_loop(struct sock *sk)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	inet_set_bit(MC_LOOP, sk);
#else
	inet_sk(sk)->mc_loop = 1;
#endif
}

static inline void ovpn_udp_clear_encap_err_rcv(struct sock *sk)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
	udp_sk(sk)->encap_err_rcv = NULL;
#endif
}

#endif /* _NET_OVPN_NET_UDP_TUNNEL_H_ */
