// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/* Do not edit directly, auto-generated from: */
/*	Documentation/netlink/specs/ovpn.yaml */
/* YNL-GEN kernel source */

#include <net/netlink.h>
#include <net/genetlink.h>

#include "netlink-gen.h"

#include <uapi/linux/ovpn.h>

/* Integer value ranges */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0) || RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8, 10)
static
#if RHEL_RELEASE_CODE > RHEL_RELEASE_VERSION(9, 4)
const
#endif
struct netlink_range_validation ovpn_a_peer_id_range = {
	.max	= 16777215ULL,
};
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0) || RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8, 10)
static
#if RHEL_RELEASE_CODE > RHEL_RELEASE_VERSION(9, 4)
const
#endif
struct netlink_range_validation ovpn_a_keyconf_peer_id_range = {
	.max	= 16777215ULL,
};
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0) && RHEL_RELEASE_CODE == 0
static int ovpn_nla_validate_range(const struct nlattr *attr,
				   struct netlink_ext_ack *extack)
{
	const u32 *value = nla_data(attr);

	if (*value > 16777215) {
		NL_SET_ERR_MSG_MOD(extack, "Value exceeds maximum");
		return -EINVAL;
	}

	return 0;
}
#endif

/* Common nested types */
const struct nla_policy ovpn_keyconf_nl_policy[OVPN_A_KEYCONF_DECRYPT_DIR + 1] = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0) || RHEL_RELEASE_CODE != 0
	[OVPN_A_KEYCONF_PEER_ID] = NLA_POLICY_FULL_RANGE(NLA_U32, &ovpn_a_keyconf_peer_id_range),
#else
	[OVPN_A_KEYCONF_PEER_ID] = NLA_POLICY_VALIDATE_FN(NLA_U32, ovpn_nla_validate_range),
#endif
	[OVPN_A_KEYCONF_SLOT] = NLA_POLICY_MAX(NLA_U32, 1),
	[OVPN_A_KEYCONF_KEY_ID] = NLA_POLICY_MAX(NLA_U32, 7),
	[OVPN_A_KEYCONF_CIPHER_ALG] = NLA_POLICY_MAX(NLA_U32, 2),
	[OVPN_A_KEYCONF_ENCRYPT_DIR] = NLA_POLICY_NESTED(ovpn_keydir_nl_policy),
	[OVPN_A_KEYCONF_DECRYPT_DIR] = NLA_POLICY_NESTED(ovpn_keydir_nl_policy),
};

const struct nla_policy ovpn_keyconf_del_input_nl_policy[OVPN_A_KEYCONF_SLOT + 1] = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0) || RHEL_RELEASE_CODE != 0
	[OVPN_A_KEYCONF_PEER_ID] = NLA_POLICY_FULL_RANGE(NLA_U32, &ovpn_a_keyconf_peer_id_range),
#else
	[OVPN_A_KEYCONF_PEER_ID] = NLA_POLICY_VALIDATE_FN(NLA_U32, ovpn_nla_validate_range),
#endif
	[OVPN_A_KEYCONF_SLOT] = NLA_POLICY_MAX(NLA_U32, 1),
};

const struct nla_policy ovpn_keyconf_get_nl_policy[OVPN_A_KEYCONF_CIPHER_ALG + 1] = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0) || RHEL_RELEASE_CODE != 0
	[OVPN_A_KEYCONF_PEER_ID] = NLA_POLICY_FULL_RANGE(NLA_U32, &ovpn_a_keyconf_peer_id_range),
#else
	[OVPN_A_KEYCONF_PEER_ID] = NLA_POLICY_VALIDATE_FN(NLA_U32, ovpn_nla_validate_range),
#endif
	[OVPN_A_KEYCONF_SLOT] = NLA_POLICY_MAX(NLA_U32, 1),
	[OVPN_A_KEYCONF_KEY_ID] = NLA_POLICY_MAX(NLA_U32, 7),
	[OVPN_A_KEYCONF_CIPHER_ALG] = NLA_POLICY_MAX(NLA_U32, 2),
};

const struct nla_policy ovpn_keyconf_swap_input_nl_policy[OVPN_A_KEYCONF_PEER_ID + 1] = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0) || RHEL_RELEASE_CODE != 0
	[OVPN_A_KEYCONF_PEER_ID] = NLA_POLICY_FULL_RANGE(NLA_U32, &ovpn_a_keyconf_peer_id_range),
#else
	[OVPN_A_KEYCONF_PEER_ID] = NLA_POLICY_VALIDATE_FN(NLA_U32, ovpn_nla_validate_range),
#endif
};

const struct nla_policy ovpn_keydir_nl_policy[OVPN_A_KEYDIR_NONCE_TAIL + 1] = {
	[OVPN_A_KEYDIR_CIPHER_KEY] = NLA_POLICY_MAX_LEN(256),
	[OVPN_A_KEYDIR_NONCE_TAIL] = NLA_POLICY_EXACT_LEN(OVPN_NONCE_TAIL_SIZE),
};

const struct nla_policy ovpn_peer_nl_policy[OVPN_A_PEER_LINK_TX_PACKETS + 1] = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0) || RHEL_RELEASE_CODE != 0
	[OVPN_A_PEER_ID] = NLA_POLICY_FULL_RANGE(NLA_U32, &ovpn_a_peer_id_range),
#else
	[OVPN_A_PEER_ID] = NLA_POLICY_VALIDATE_FN(NLA_U32, ovpn_nla_validate_range),
#endif
	[OVPN_A_PEER_REMOTE_IPV4] = { .type = NLA_BE32, },
	[OVPN_A_PEER_REMOTE_IPV6] = NLA_POLICY_EXACT_LEN(16),
	[OVPN_A_PEER_REMOTE_IPV6_SCOPE_ID] = { .type = NLA_U32, },
	[OVPN_A_PEER_REMOTE_PORT] = NLA_POLICY_MIN(NLA_BE16, 1),
	[OVPN_A_PEER_SOCKET] = { .type = NLA_U32, },
	[OVPN_A_PEER_SOCKET_NETNSID] = { .type = NLA_S32, },
	[OVPN_A_PEER_VPN_IPV4] = { .type = NLA_BE32, },
	[OVPN_A_PEER_VPN_IPV6] = NLA_POLICY_EXACT_LEN(16),
	[OVPN_A_PEER_LOCAL_IPV4] = { .type = NLA_BE32, },
	[OVPN_A_PEER_LOCAL_IPV6] = NLA_POLICY_EXACT_LEN(16),
	[OVPN_A_PEER_LOCAL_PORT] = NLA_POLICY_MIN(NLA_BE16, 1),
	[OVPN_A_PEER_KEEPALIVE_INTERVAL] = { .type = NLA_U32, },
	[OVPN_A_PEER_KEEPALIVE_TIMEOUT] = { .type = NLA_U32, },
	[OVPN_A_PEER_DEL_REASON] = NLA_POLICY_MAX(NLA_U32, 4),
	[OVPN_A_PEER_VPN_RX_BYTES] = { .type = NLA_UINT, },
	[OVPN_A_PEER_VPN_TX_BYTES] = { .type = NLA_UINT, },
	[OVPN_A_PEER_VPN_RX_PACKETS] = { .type = NLA_UINT, },
	[OVPN_A_PEER_VPN_TX_PACKETS] = { .type = NLA_UINT, },
	[OVPN_A_PEER_LINK_RX_BYTES] = { .type = NLA_UINT, },
	[OVPN_A_PEER_LINK_TX_BYTES] = { .type = NLA_UINT, },
	[OVPN_A_PEER_LINK_RX_PACKETS] = { .type = NLA_UINT, },
	[OVPN_A_PEER_LINK_TX_PACKETS] = { .type = NLA_UINT, },
};

const struct nla_policy ovpn_peer_del_input_nl_policy[OVPN_A_PEER_ID + 1] = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0) || RHEL_RELEASE_CODE != 0
	[OVPN_A_PEER_ID] = NLA_POLICY_FULL_RANGE(NLA_U32, &ovpn_a_peer_id_range),
#else
	[OVPN_A_PEER_ID] = NLA_POLICY_VALIDATE_FN(NLA_U32, ovpn_nla_validate_range),
#endif
};

const struct nla_policy ovpn_peer_new_input_nl_policy[OVPN_A_PEER_KEEPALIVE_TIMEOUT + 1] = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0) || RHEL_RELEASE_CODE != 0
	[OVPN_A_PEER_ID] = NLA_POLICY_FULL_RANGE(NLA_U32, &ovpn_a_peer_id_range),
#else
	[OVPN_A_PEER_ID] = NLA_POLICY_VALIDATE_FN(NLA_U32, ovpn_nla_validate_range),
#endif
	[OVPN_A_PEER_REMOTE_IPV4] = { .type = NLA_BE32, },
	[OVPN_A_PEER_REMOTE_IPV6] = NLA_POLICY_EXACT_LEN(16),
	[OVPN_A_PEER_REMOTE_IPV6_SCOPE_ID] = { .type = NLA_U32, },
	[OVPN_A_PEER_REMOTE_PORT] = NLA_POLICY_MIN(NLA_BE16, 1),
	[OVPN_A_PEER_SOCKET] = { .type = NLA_U32, },
	[OVPN_A_PEER_VPN_IPV4] = { .type = NLA_BE32, },
	[OVPN_A_PEER_VPN_IPV6] = NLA_POLICY_EXACT_LEN(16),
	[OVPN_A_PEER_LOCAL_IPV4] = { .type = NLA_BE32, },
	[OVPN_A_PEER_LOCAL_IPV6] = NLA_POLICY_EXACT_LEN(16),
	[OVPN_A_PEER_KEEPALIVE_INTERVAL] = { .type = NLA_U32, },
	[OVPN_A_PEER_KEEPALIVE_TIMEOUT] = { .type = NLA_U32, },
};

const struct nla_policy ovpn_peer_set_input_nl_policy[OVPN_A_PEER_KEEPALIVE_TIMEOUT + 1] = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0) || RHEL_RELEASE_CODE != 0
	[OVPN_A_PEER_ID] = NLA_POLICY_FULL_RANGE(NLA_U32, &ovpn_a_peer_id_range),
#else
	[OVPN_A_PEER_ID] = NLA_POLICY_VALIDATE_FN(NLA_U32, ovpn_nla_validate_range),
#endif
	[OVPN_A_PEER_REMOTE_IPV4] = { .type = NLA_BE32, },
	[OVPN_A_PEER_REMOTE_IPV6] = NLA_POLICY_EXACT_LEN(16),
	[OVPN_A_PEER_REMOTE_IPV6_SCOPE_ID] = { .type = NLA_U32, },
	[OVPN_A_PEER_REMOTE_PORT] = NLA_POLICY_MIN(NLA_BE16, 1),
	[OVPN_A_PEER_VPN_IPV4] = { .type = NLA_BE32, },
	[OVPN_A_PEER_VPN_IPV6] = NLA_POLICY_EXACT_LEN(16),
	[OVPN_A_PEER_LOCAL_IPV4] = { .type = NLA_BE32, },
	[OVPN_A_PEER_LOCAL_IPV6] = NLA_POLICY_EXACT_LEN(16),
	[OVPN_A_PEER_KEEPALIVE_INTERVAL] = { .type = NLA_U32, },
	[OVPN_A_PEER_KEEPALIVE_TIMEOUT] = { .type = NLA_U32, },
};

/* OVPN_CMD_PEER_NEW - do */
static const struct nla_policy ovpn_peer_new_nl_policy[OVPN_A_PEER + 1] = {
	[OVPN_A_IFINDEX] = { .type = NLA_U32, },
	[OVPN_A_PEER] = NLA_POLICY_NESTED(ovpn_peer_new_input_nl_policy),
};

/* OVPN_CMD_PEER_SET - do */
static const struct nla_policy ovpn_peer_set_nl_policy[OVPN_A_PEER + 1] = {
	[OVPN_A_IFINDEX] = { .type = NLA_U32, },
	[OVPN_A_PEER] = NLA_POLICY_NESTED(ovpn_peer_set_input_nl_policy),
};

/* OVPN_CMD_PEER_GET - do */
static const struct nla_policy ovpn_peer_get_do_nl_policy[OVPN_A_PEER + 1] = {
	[OVPN_A_IFINDEX] = { .type = NLA_U32, },
	[OVPN_A_PEER] = NLA_POLICY_NESTED(ovpn_peer_nl_policy),
};

/* OVPN_CMD_PEER_GET - dump */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0) || RHEL_RELEASE_CODE != 0
static
#endif
const struct nla_policy ovpn_peer_get_dump_nl_policy[OVPN_A_IFINDEX + 1] = {
	[OVPN_A_IFINDEX] = { .type = NLA_U32, },
};

/* OVPN_CMD_PEER_DEL - do */
static const struct nla_policy ovpn_peer_del_nl_policy[OVPN_A_PEER + 1] = {
	[OVPN_A_IFINDEX] = { .type = NLA_U32, },
	[OVPN_A_PEER] = NLA_POLICY_NESTED(ovpn_peer_del_input_nl_policy),
};

/* OVPN_CMD_KEY_NEW - do */
static const struct nla_policy ovpn_key_new_nl_policy[OVPN_A_KEYCONF + 1] = {
	[OVPN_A_IFINDEX] = { .type = NLA_U32, },
	[OVPN_A_KEYCONF] = NLA_POLICY_NESTED(ovpn_keyconf_nl_policy),
};

/* OVPN_CMD_KEY_GET - do */
static const struct nla_policy ovpn_key_get_nl_policy[OVPN_A_KEYCONF + 1] = {
	[OVPN_A_IFINDEX] = { .type = NLA_U32, },
	[OVPN_A_KEYCONF] = NLA_POLICY_NESTED(ovpn_keyconf_get_nl_policy),
};

/* OVPN_CMD_KEY_SWAP - do */
static const struct nla_policy ovpn_key_swap_nl_policy[OVPN_A_KEYCONF + 1] = {
	[OVPN_A_IFINDEX] = { .type = NLA_U32, },
	[OVPN_A_KEYCONF] = NLA_POLICY_NESTED(ovpn_keyconf_swap_input_nl_policy),
};

/* OVPN_CMD_KEY_DEL - do */
static const struct nla_policy ovpn_key_del_nl_policy[OVPN_A_KEYCONF + 1] = {
	[OVPN_A_IFINDEX] = { .type = NLA_U32, },
	[OVPN_A_KEYCONF] = NLA_POLICY_NESTED(ovpn_keyconf_del_input_nl_policy),
};

/* Ops table for ovpn */
static const struct genl_split_ops ovpn_nl_ops[] = {
	{
		.cmd		= OVPN_CMD_PEER_NEW,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0) || RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9, 3)
		.pre_doit	= ovpn_nl_pre_doit,
		.post_doit	= ovpn_nl_post_doit,
#endif
		.doit		= ovpn_nl_peer_new_doit,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) || LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0) || RHEL_RELEASE_CODE != 0
		.policy		= ovpn_peer_new_nl_policy,
		.maxattr	= OVPN_A_PEER,
#endif
		.flags		= GENL_ADMIN_PERM | GENL_CMD_CAP_DO,
	},
	{
		.cmd		= OVPN_CMD_PEER_SET,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0) || RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9, 3)
		.pre_doit	= ovpn_nl_pre_doit,
		.post_doit	= ovpn_nl_post_doit,
#endif
		.doit		= ovpn_nl_peer_set_doit,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) || LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0) || RHEL_RELEASE_CODE != 0
		.policy		= ovpn_peer_set_nl_policy,
		.maxattr	= OVPN_A_PEER,
#endif
		.flags		= GENL_ADMIN_PERM | GENL_CMD_CAP_DO,
	},
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0) || RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9, 3)
	{
		.cmd		= OVPN_CMD_PEER_GET,
		.pre_doit	= ovpn_nl_pre_doit,
		.doit		= ovpn_nl_peer_get_doit,
		.post_doit	= ovpn_nl_post_doit,
		.policy		= ovpn_peer_get_do_nl_policy,
		.maxattr	= OVPN_A_PEER,
		.flags		= GENL_ADMIN_PERM | GENL_CMD_CAP_DO,
	},
	{
		.cmd		= OVPN_CMD_PEER_GET,
		.dumpit		= ovpn_nl_peer_get_dumpit,
		.policy		= ovpn_peer_get_dump_nl_policy,
		.maxattr	= OVPN_A_IFINDEX,
		.flags		= GENL_ADMIN_PERM | GENL_CMD_CAP_DUMP,
	},
#else
	{
		.cmd		= OVPN_CMD_PEER_GET,
		.doit		= ovpn_nl_peer_get_doit,
		.dumpit		= ovpn_nl_peer_get_dumpit,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) || LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0)
		.policy		= ovpn_peer_get_do_nl_policy,
		.maxattr	= OVPN_A_PEER,
#endif
		.flags		= GENL_ADMIN_PERM | GENL_CMD_CAP_DO | GENL_CMD_CAP_DUMP,
	},
#endif
	{
		.cmd		= OVPN_CMD_PEER_DEL,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0) || RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9, 3)
		.pre_doit	= ovpn_nl_pre_doit,
		.post_doit	= ovpn_nl_post_doit,
#endif
		.doit		= ovpn_nl_peer_del_doit,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) || LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0) || RHEL_RELEASE_CODE != 0
		.policy		= ovpn_peer_del_nl_policy,
		.maxattr	= OVPN_A_PEER,
#endif
		.flags		= GENL_ADMIN_PERM | GENL_CMD_CAP_DO,
	},
	{
		.cmd		= OVPN_CMD_KEY_NEW,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0) || RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9, 3)
		.pre_doit	= ovpn_nl_pre_doit,
		.post_doit	= ovpn_nl_post_doit,
#endif
		.doit		= ovpn_nl_key_new_doit,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) || LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0) || RHEL_RELEASE_CODE != 0
		.policy		= ovpn_key_new_nl_policy,
		.maxattr	= OVPN_A_KEYCONF,
#endif
		.flags		= GENL_ADMIN_PERM | GENL_CMD_CAP_DO,
	},
	{
		.cmd		= OVPN_CMD_KEY_GET,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0) || RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9, 3)
		.pre_doit	= ovpn_nl_pre_doit,
		.post_doit	= ovpn_nl_post_doit,
#endif
		.doit		= ovpn_nl_key_get_doit,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) || LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0) || RHEL_RELEASE_CODE != 0
		.policy		= ovpn_key_get_nl_policy,
		.maxattr	= OVPN_A_KEYCONF,
#endif
		.flags		= GENL_ADMIN_PERM | GENL_CMD_CAP_DO,
	},
	{
		.cmd		= OVPN_CMD_KEY_SWAP,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0) || RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9, 3)
		.pre_doit	= ovpn_nl_pre_doit,
		.post_doit	= ovpn_nl_post_doit,
#endif
		.doit		= ovpn_nl_key_swap_doit,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) || LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0) || RHEL_RELEASE_CODE != 0
		.policy		= ovpn_key_swap_nl_policy,
		.maxattr	= OVPN_A_KEYCONF,
#endif
		.flags		= GENL_ADMIN_PERM | GENL_CMD_CAP_DO,
	},
	{
		.cmd		= OVPN_CMD_KEY_DEL,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0) || RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9, 3)
		.pre_doit	= ovpn_nl_pre_doit,
		.post_doit	= ovpn_nl_post_doit,
#endif
		.doit		= ovpn_nl_key_del_doit,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) || LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0) || RHEL_RELEASE_CODE != 0
		.policy		= ovpn_key_del_nl_policy,
		.maxattr	= OVPN_A_KEYCONF,
#endif
		.flags		= GENL_ADMIN_PERM | GENL_CMD_CAP_DO,
	},
};

static const struct genl_multicast_group ovpn_nl_mcgrps[] = {
	[OVPN_NLGRP_PEERS] = { "peers", },
};

struct genl_family ovpn_nl_family __ro_after_init = {
	.name		= OVPN_FAMILY_NAME,
	.version	= OVPN_FAMILY_VERSION,
	.netnsok	= true,
	.parallel_ops	= true,
	.module		= THIS_MODULE,
	.split_ops	= ovpn_nl_ops,
	.n_split_ops	= ARRAY_SIZE(ovpn_nl_ops),
	.mcgrps		= ovpn_nl_mcgrps,
	.n_mcgrps	= ARRAY_SIZE(ovpn_nl_mcgrps),
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0) && RHEL_RELEASE_CODE < RHEL_RELEASE_VERSION(9, 3)
	.pre_doit	= ovpn_nl_pre_doit,
	.post_doit	= ovpn_nl_post_doit,
	.maxattr	= OVPN_A_MAX,
#endif
};
