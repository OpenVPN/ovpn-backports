#ifndef _NET_OVPN_LINUX_SKBUFF_H_
#define _NET_OVPN_LINUX_SKBUFF_H_

#include <linux/version.h>
#include_next <linux/skbuff.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 10)

static inline void skb_mark_not_on_list(struct sk_buff *skb)
{
	skb->next = NULL;
}

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 10) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 10)

static inline unsigned int skb_frag_off(const skb_frag_t *frag)
{
	return frag->page_offset;
}

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 10) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 5, 0) && \
	SUSE_PRODUCT_CODE < SUSE_PRODUCT(1, 15, 6, 0)

int kernel_sendmsg_locked(struct sock *sk, struct msghdr *msg, struct kvec *vec,
			  size_t num, size_t size);
int kernel_sendpage_locked(struct sock *sk, struct page *page, int offset,
			   size_t size, int flags);

#elif LINUX_VERSION_CODE < KERNEL_VERSION(6, 16, 0)

static int ovpn_sendmsg_locked(struct sock *sk, struct msghdr *msg);

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 5, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 16, 0)

static inline int ovpn_skb_send_sock_locked_with_flags(struct sock *sk,
						       struct sk_buff *skb,
						       int offset, int len,
						       int flags)
{
	unsigned int orig_len = len;
	struct sk_buff *head = skb;
	unsigned short fragidx;
	int slen, ret;

do_frag_list:

	/* Deal with head data */
	while (offset < skb_headlen(skb) && len) {
		struct kvec kv;
		struct msghdr msg;

		slen = min_t(int, len, skb_headlen(skb) - offset);
		kv.iov_base = skb->data + offset;
		kv.iov_len = slen;
		memset(&msg, 0, sizeof(msg));
		msg.msg_flags = MSG_DONTWAIT | flags;

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 5, 0) && \
	SUSE_PRODUCT_CODE < SUSE_PRODUCT(1, 15, 6, 0)
		ret = kernel_sendmsg_locked(sk, &msg, &kv, 1, slen);
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION (6, 5, 0) */
		iov_iter_kvec(&msg.msg_iter, ITER_SOURCE, &kv, 1, slen);
		ret = ovpn_sendmsg_locked(sk, &msg);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 5, 0) */
		if (ret <= 0)
			goto error;

		offset += ret;
		len -= ret;
	}

	/* All the data was skb head? */
	if (!len)
		goto out;

	/* Make offset relative to start of frags */
	offset -= skb_headlen(skb);

	/* Find where we are in frag list */
	for (fragidx = 0; fragidx < skb_shinfo(skb)->nr_frags; fragidx++) {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[fragidx];

		if (offset < skb_frag_size(frag))
			break;

		offset -= skb_frag_size(frag);
	}

	for (; len && fragidx < skb_shinfo(skb)->nr_frags; fragidx++) {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[fragidx];

		slen = min_t(size_t, len, skb_frag_size(frag) - offset);

		while (slen) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 5, 0) && \
	SUSE_PRODUCT_CODE < SUSE_PRODUCT(1, 15, 6, 0)
			ret = kernel_sendpage_locked(
				sk, skb_frag_page(frag),
				skb_frag_off(frag) + offset, slen,
				MSG_DONTWAIT | flags);
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(6, 5, 0) */
			struct bio_vec bvec;
			struct msghdr msg = {
				.msg_flags = MSG_SPLICE_PAGES | MSG_DONTWAIT |
					     flags,
			};

			bvec_set_page(&bvec, skb_frag_page(frag), slen,
				      skb_frag_off(frag) + offset);
			iov_iter_bvec(&msg.msg_iter, ITER_SOURCE, &bvec, 1,
				      slen);

			ret = ovpn_sendmsg_locked(sk, &msg);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 5, 0) */
			if (ret <= 0)
				goto error;

			len -= ret;
			offset += ret;
			slen -= ret;
		}

		offset = 0;
	}

	if (len) {
		/* Process any frag lists */

		if (skb == head) {
			if (skb_has_frag_list(skb)) {
				skb = skb_shinfo(skb)->frag_list;
				goto do_frag_list;
			}
		} else if (skb->next) {
			skb = skb->next;
			goto do_frag_list;
		}
	}

out:
	return orig_len - len;

error:
	return orig_len == len ? ret : orig_len - len;
}

#define skb_send_sock_locked_with_flags(sk, skb, offset, len, flags)       \
	ovpn_skb_send_sock_locked_with_flags((sk), (skb), (offset), (len), \
					     (flags))

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 16, 0) */

#endif /* _NET_OVPN_LINUX_SKBUFF_H_ */
