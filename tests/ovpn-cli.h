#ifndef _OVPN_TESTS_OVPN_CLI_H_
#define _OVPN_TESTS_OVPN_CLI_H_

#include <linux/if_link.h>

#ifndef IFLA_OVPN_MAX

enum ovpn_mode {
	OVPN_MODE_P2P,
	OVPN_MODE_MP,
};

enum {
	IFLA_OVPN_UNSPEC,
	IFLA_OVPN_MODE,
	__IFLA_OVPN_MAX,
};

#define IFLA_OVPN_MAX (__IFLA_OVPN_MAX - 1)

#endif /* IFLA_OVPN_MAX */

#endif /* _OVPN_TESTS_OVPN_CLI_H_ */
