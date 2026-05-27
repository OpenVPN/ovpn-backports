#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

set -euo pipefail

script_dir=$(unset CDPATH; cd -- "$(dirname -- "$0")" && pwd)

# shellcheck source=ci/rootfs-common.sh
. "${script_dir}/rootfs-common.sh"

usage() {
	echo "Usage: $0 <target> <rootfs-dir> <repo-dir>" >&2
	exit 1
}

if [ "$#" -ne 3 ]; then
	usage
fi

distro="$1"
rootfs=$(realpath "$2")
repo=$(realpath "$3")
force_9p=0
root_guesttools=0

case "${distro}" in
debian-10)
	# debian10 is too old for virtiofs
	force_9p=1
	echo "${distro} requires 9p rootfs"
	;;
debian-11|debian-12|debian-13|ubuntu-20.04|ubuntu-22.04|ubuntu-24.04|ubuntu-25.10|ubuntu-26.04|fedora-44|opensuse-leap-15.6|opensuse-leap-16.0|opensuse-tumbleweed)
	;;
alma-8|alma-9|alma-10)
	root_guesttools=1
	;;
*)
	echo "Unsupported target: ${distro}" >&2
	usage
	;;
esac

if [ ! -d "${rootfs}" ]; then
	echo "Missing rootfs: ${rootfs}" >&2
	exit 1
fi

if [ ! -x "${repo}/ci/guest-run-selftests.sh" ]; then
	echo "Missing guest test script in ${repo}" >&2
	exit 1
fi

kernel=$(rootfs_find_kernel_image "${rootfs}")

if [ -z "${kernel}" ]; then
	echo "No kernel image found in ${rootfs}" >&2
	exit 1
fi

kernel_release=$(rootfs_kernel_release "${kernel}")

if ! rootfs_has_kernel_headers "${rootfs}" "${kernel_release}"; then
	echo "Missing headers for ${kernel_release}" >&2
	exit 1
fi

vng_args=()
vng_cmd="${VNG:-vng}"
guest_repo="/repo"

# Luckily GH workers support KVM acceleration so nested virtualization is
# smooth. If that changes in the future, we fallback to QEMU though that's
# painfully slow.
if [ -e /dev/kvm ]; then
	echo "Using KVM acceleration"
else
	echo "KVM unavailable, using QEMU emulation"
	vng_args+=(--disable-kvm)
fi
if [ "${VNG_VERBOSE:-1}" = "1" ]; then
	vng_args+=(--verbose)
fi
if [ "${force_9p}" -eq 1 ]; then
	echo "Forcing 9p rootfs in vng"
	vng_args+=(--force-9p)
fi

# copy the ovpn-backports repo into the rootfs
rsync -a --delete "${repo}/" "${rootfs}${guest_repo}/"

# tell vng to copy its guesttools into the rootfs and execute them from there
if [ "${root_guesttools}" -eq 1 ]; then
	vng_args+=(--root-guesttools --no-virtme-ng-init)
fi

echo "Booting ${distro} with ${kernel_release}"

"${vng_cmd}" \
	--run "${kernel}" \
	--root "${rootfs}" \
	--rw \
	--user root \
	"${vng_args[@]}" \
	--disable-microvm \
	--force-initramfs \
	--cpus "${VNG_CPUS:-2}" \
	--memory "${VNG_MEMORY:-4096M}" \
	-- \
	"${guest_repo}/ci/guest-run-selftests.sh"
