#!/bin/bash

set -e

KERNEL_REPO_URL='https://github.com/OpenVPN/ovpn-net-next.git'
KERNEL_COMMIT=${KERNEL_COMMIT:-'f2dfcc4b4bc28ba8ad45bce43ad76fa9575e27f5'}
KERNEL_DIR="$PWD/kernel"

clean_ovpn_selftests() {
	rm -fr \
		"$PWD/tools/testing/selftests/net/ovpn" \
		"$PWD/tools/testing/selftests/kselftest" \
		"$PWD/tools/testing/selftests/Makefile" \
		"$PWD/tools/testing/selftests/lib.mk" \
		"$PWD/tools/testing/selftests/kselftest.h" \
		"$PWD/tools/testing/selftests/kselftest_harness.h" \
		"$PWD/tools/testing/selftests/run_kselftest.sh" \
		"$PWD/tools/net/ynl/pyynl" \
		"$PWD/Documentation/netlink/specs/ovpn.yaml" \
		"$PWD/Documentation/netlink/genetlink.yaml" \
		"$PWD/scripts/subarch.include"
}

import_ovpn_selftests() {
	echo "Extracting ovpn selftests"
	clean_ovpn_selftests
	mkdir -p \
		"$PWD/tools/testing/selftests/net" \
		"$PWD/tools/net/ynl" \
		"$PWD/Documentation/netlink/specs" \
		"$PWD/scripts"

	cp -r "$KERNEL_DIR/tools/testing/selftests/net/ovpn" \
		"$PWD/tools/testing/selftests/net/"
	cp -r "$KERNEL_DIR/tools/testing/selftests/kselftest" \
		"$PWD/tools/testing/selftests/"
	cp "$KERNEL_DIR/tools/testing/selftests/Makefile" \
		"$PWD/tools/testing/selftests/Makefile"
	cp "$KERNEL_DIR/tools/testing/selftests/lib.mk" \
		"$PWD/tools/testing/selftests/lib.mk"
	cp "$KERNEL_DIR/tools/testing/selftests/kselftest.h" \
		"$PWD/tools/testing/selftests/kselftest.h"
	cp "$KERNEL_DIR/tools/testing/selftests/kselftest_harness.h" \
		"$PWD/tools/testing/selftests/kselftest_harness.h"
	cp "$KERNEL_DIR/tools/testing/selftests/run_kselftest.sh" \
		"$PWD/tools/testing/selftests/run_kselftest.sh"
	cp -r "$KERNEL_DIR/tools/net/ynl/pyynl" "$PWD/tools/net/ynl/"
	cp "$KERNEL_DIR/Documentation/netlink/specs/ovpn.yaml" \
		"$PWD/Documentation/netlink/specs/ovpn.yaml"
	cp "$KERNEL_DIR/Documentation/netlink/genetlink.yaml" \
		"$PWD/Documentation/netlink/genetlink.yaml"
	cp "$KERNEL_DIR/scripts/subarch.include" "$PWD/scripts/subarch.include"

	for patch in "$PWD"/compat-patches/tests/*.patch; do
		[ -e "$patch" ] || continue
		git apply --verbose "$patch"
	done
	for patch in "$PWD"/compat-patches/ynl/*.patch; do
		[ -e "$patch" ] || continue
		git apply --verbose "$patch"
	done
}

get_ovpn() {
	keep=$1
	import_tests=$2

	if [[ ! -d $KERNEL_DIR ]]; then
		git clone --depth 1 "$KERNEL_REPO_URL" "$KERNEL_DIR"
	fi

	echo "Checking out commit $KERNEL_COMMIT"
	git -C "$KERNEL_DIR" fetch --depth 1 origin "$KERNEL_COMMIT"
	git -C "$KERNEL_DIR" reset --hard "$KERNEL_COMMIT"

	echo "Extracting ovpn source files"
	rm -fr "$PWD/drivers/" "$PWD/include/"
	clean_ovpn_selftests
	mkdir -p "$PWD/drivers/net" "$PWD/include/uapi/linux"
	cp -r "$KERNEL_DIR/drivers/net/ovpn" "$PWD/drivers/net/"
	cp "$KERNEL_DIR/include/uapi/linux/ovpn.h" "$PWD/include/uapi/linux/ovpn.h"

	for patch in "$PWD"/compat-patches/sources/*.patch; do
		git apply --verbose "$patch"
	done

	if [ "$import_tests" -eq "1" ]; then
		import_ovpn_selftests
	fi

	# We extract the branch from backports because all the non-sources branches
	# of this repo point directly to the corresponding branch in ovpn-net-next
	# and there's no way of getting this info from ovpn-net-next since it has
	# been cloned with --depth 1.
	branch=$(git rev-parse --abbrev-ref HEAD)

	# Save version information to a file (as key=value pairs) unless we're in a
	# sources branch.
	if [[ $branch != *'sources'* ]]; then
		echo "Setting version information"

		# Name of the repository from where the ovpn sources were extracted.
		tree=$(basename "$(git -C "$KERNEL_DIR" config --get remote.origin.url)" | cut -d. -f1)

		# Version of the kernel from where the ovpn sources were extracted.
		kernel_version=$(make -s -C "$KERNEL_DIR" kernelversion)

		# This indirectly indicates also the ovpn-net-next commit used for
		# generating the backports.
		backports_commit=$(git rev-parse --short HEAD)

		rm -f "$PWD/.version"
		cat << EOF > "$PWD/.version"
tree=${tree}
branch=${branch}
kernel_version=${kernel_version}
backports_commit=${backports_commit}
EOF
	fi

	if [ "$keep" -eq "0" ] ; then
		echo "Cleaning up"
		rm -rf "$KERNEL_DIR"
	fi
}

print_usage() {
	echo "Usage: ./backports-ctl.sh <get-ovpn|clean>"
	echo "       ./backports-ctl.sh get-ovpn [-k|--keep] [-t|--tests]"
	exit 1
}

if ! git --version >/dev/null 2>&1; then
	echo "git could not be found"
	exit 1
fi

command="$1"
if [[ -z $command || $command = "help" ]]; then
	print_usage
elif [ "$command" = "get-ovpn" ]; then
	shift
	keep=0
	import_tests=0
	while [ "$#" -gt 0 ]; do
		case "$1" in
		-k|--keep)
			keep=1
			;;
		-t|--tests)
			import_tests=1
			;;
		*)
			print_usage
			;;
		esac
		shift
	done
	get_ovpn "$keep" "$import_tests"
elif [ "$command" = "clean" ]; then
	read -r -p "Are you sure you want to restore the repository to its default state? [y/N]" clean && [[ "$clean" = "y" || "$clean" = "Y" ]] && git clean -fdx && git reset --hard
else
	echo "Unknown command $command"
	print_usage
fi
