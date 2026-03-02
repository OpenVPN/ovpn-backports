#ifndef _NET_OVPN_NET_NETLINK_H_
#define _NET_OVPN_NET_NETLINK_H_

#include <linux/version.h>
#include_next <net/netlink.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0)

#ifndef __NLA_ENSURE
#define __NLA_ENSURE(condition) (0)
#endif /* __NLA_ENSURE */

#ifndef NLA_ENSURE_INT_TYPE
#define NLA_ENSURE_INT_TYPE(tp) { .type = tp }
#endif /* NLA_ENSURE_INT_TYPE */

#ifndef NLA_POLICY_RANGE
#define NLA_POLICY_RANGE(tp, _min, _max) { .type = tp }
#endif /* NLA_POLICY_RANGE */

#ifndef NLA_POLICY_MIN
#define NLA_POLICY_MIN(tp, _min) { .type = tp }
#endif /* NLA_POLICY_MIN */

#ifndef NLA_POLICY_MAX
#define NLA_POLICY_MAX(tp, _max) { .type = tp }
#endif /* NLA_POLICY_MAX */

#ifndef NLA_POLICY_EXACT_LEN
#define NLA_POLICY_EXACT_LEN(_len) { .len = _len }
#endif /* NLA_POLICY_EXACT_LEN */

#ifndef NLA_POLICY_VALIDATE_FN
#define NLA_POLICY_VALIDATE_FN(tp, _range) { .type = tp }
#endif /* NLA_POLICY_VALIDATE_FN */

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(4, 20, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0)

#ifndef _NLA_POLICY_NESTED
#define _NLA_POLICY_NESTED(maxattr, policy) \
	{ .type = NLA_NESTED, .validation_data = (void *)(policy), .len = maxattr }
#endif /* _NLA_POLICY_NESTED */

#ifndef _NLA_POLICY_NESTED_ARRAY
#define _NLA_POLICY_NESTED_ARRAY(maxattr, policy) \
	{ .type = NLA_NESTED_ARRAY, .validation_data = (void *)(policy), .len = maxattr }
#endif /* _NLA_POLICY_NESTED_ARRAY */

#if RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 10)
#undef NLA_POLICY_NESTED
#define NLA_POLICY_NESTED(policy) \
	_NLA_POLICY_NESTED(ARRAY_SIZE(policy) - 1, policy)

#undef NLA_POLICY_NESTED_ARRAY
#define NLA_POLICY_NESTED_ARRAY(policy) \
	_NLA_POLICY_NESTED_ARRAY(ARRAY_SIZE(policy) - 1, policy)
#endif /* RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 10) */

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 10)

struct netlink_range_validation {
	u64 min, max;
};

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 10) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)

#ifndef NLA_ENSURE_UINT_OR_BINARY_TYPE
#define NLA_ENSURE_UINT_OR_BINARY_TYPE(tp)		\
	(__NLA_ENSURE(tp == NLA_U8 || tp == NLA_U16 ||	\
		      tp == NLA_U32 || tp == NLA_U64 ||	\
		      tp == NLA_MSECS ||		\
		      tp == NLA_BINARY) + tp)
#endif /* NLA_ENSURE_UINT_OR_BINARY_TYPE */

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 13, 0)

#ifndef NLA_POLICY_MAX_LEN
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
#define NLA_POLICY_MAX_LEN(_len) { .type = NLA_BINARY, .len = _len }
#else
#define NLA_POLICY_MAX_LEN(_len) NLA_POLICY_MAX(NLA_BINARY, _len)
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0) */
#endif /* NLA_POLICY_MAX_LEN */

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 13, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 7, 0) && \
	RHEL_RELEASE_CODE <= RHEL_RELEASE_VERSION(9, 4)

static inline int ovpn_nla_put_uint(struct sk_buff *skb, int attrtype,
				    u64 value)
{
	u64 tmp64 = value;
	u32 tmp32 = value;

	if (tmp64 == tmp32)
		return nla_put_u32(skb, attrtype, tmp32);
	return nla_put(skb, attrtype, sizeof(u64), &tmp64);
}

#define nla_put_uint ovpn_nla_put_uint

#ifndef NLA_UINT
#define NLA_UINT NLA_U64
#endif /* NLA_UINT */

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 7, 0) && RHEL_RELEASE_CODE <= RHEL_RELEASE_VERSION(9, 4) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 7, 0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 10)

#undef NLA_POLICY_FULL_RANGE
#define NLA_POLICY_FULL_RANGE(tp, _range)                           \
	{                                                           \
		.type = NLA_ENSURE_UINT_OR_BINARY_TYPE(tp),         \
		.validation_type = NLA_VALIDATE_RANGE_PTR,          \
		.range = (struct netlink_range_validation *)_range, \
	}

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 7, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(8, 10) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0) && \
	RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(9, 5)

#define NLA_BE16 NLA_U16
#define NLA_BE32 NLA_U32

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(9, 5) */

#endif /* _NET_OVPN_NET_NETLINK_H_ */
