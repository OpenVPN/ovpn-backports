#!/bin/bash

set -e

KERNEL_REPO_URL='https://github.com/OpenVPN/ovpn-net-next.git'
KERNEL_COMMIT=${KERNEL_COMMIT:-'07fdad3a93756b872da7b53647715c48d0f4a2d0'}
KERNEL_DIR="$PWD/kernel"

get_ovpn() {
	keep=$1

	if [[ ! -d $KERNEL_DIR ]]; then
		git clone --depth 1 $KERNEL_REPO_URL $KERNEL_DIR
	fi

	echo "Checking out commit $KERNEL_COMMIT"
	git -C $KERNEL_DIR fetch --depth 1 origin $KERNEL_COMMIT
	git -C $KERNEL_DIR reset --hard $KERNEL_COMMIT

	echo "Extracting ovpn source files"
	rm -fr $PWD/drivers/ $PWD/include/ $PWD/tests/ovpn-cli
	mkdir -p $PWD/drivers/net $PWD/include/uapi/linux
	cp -r $KERNEL_DIR/drivers/net/ovpn $PWD/drivers/net/
	cp $KERNEL_DIR/include/uapi/linux/ovpn.h $PWD/include/uapi/linux/ovpn.h
	cp -r $KERNEL_DIR/tools/testing/selftests/net/ovpn $PWD/tests/ovpn-cli

	for patch in $PWD/compat-patches/sources/*.patch; do
		git apply --verbose "$patch"
	done

	for patch in $PWD/compat-patches/tests/*.patch; do
		git apply --verbose "$patch"
	done

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
		tree=$(basename $(git -C $KERNEL_DIR config --get remote.origin.url) | cut -d. -f1)

		# Version of the kernel from where the ovpn sources were extracted.
		kernel_version=$(make -s -C $KERNEL_DIR kernelversion)

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
		rm -rf $KERNEL_DIR
	fi
}

print_usage() {
	echo "Usage: ./backports-ctl.sh <get-ovpn|clean>"
	exit 1
}

if ! git --version 2>&1 >/dev/null; then
	echo "git could not be found"
	exit 1
fi

command="$1"
if [[ -z $command || $command = "help" ]]; then
	print_usage
elif [ $command = "get-ovpn" ]; then
	keep="$2"
	if [ -z "$keep" ]; then
		get_ovpn 0
	elif [[ "$keep" = "-k" || "$keep" = "--keep" ]]; then
		get_ovpn 1
	else
		echo "Usage $0 get-ovpn [-k | --keep]"
	fi
elif [ $command = "clean" ]; then
	read -p "Are you sure you want to restore the repository to its default state? [y/N]" clean && [[ "$clean" = "y" || "$clean" = "Y" ]] && git clean -fdx && git reset --hard
else
	echo "Unknown command $command"
	print_usage
fi
