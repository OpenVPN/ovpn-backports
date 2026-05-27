#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

# Return the newest kernel image installed in a generated rootfs. Prefer /boot,
# but keep a fallback for RPM-style rootfs layouts that also ship a module-local
# kernel image.
rootfs_find_kernel_image() {
	local rootfs="$1"
	local kernel=""

	if [ -d "${rootfs}/boot" ]; then
		kernel=$(find "${rootfs}/boot" -maxdepth 1 -type f -name 'vmlinuz-*' |
			sort -V | tail -n1)
	fi

	if [ -n "${kernel}" ]; then
		printf '%s\n' "${kernel}"
		return
	fi

	if [ -d "${rootfs}/lib/modules" ]; then
		kernel=$(find "${rootfs}/lib/modules" -mindepth 2 -maxdepth 2 \
			-type f -name vmlinuz | sort -V | tail -n1)
	fi

	printf '%s\n' "${kernel}"
}

# Derive the kernel release string from the image path layout used by the
# supported distros.
rootfs_kernel_release() {
	local kernel="$1"
	local release

	case "${kernel}" in
	# /boot/vmlinuz-<release>, used by the normal kernel image packages in
	# the current Debian, Ubuntu, Fedora, AlmaLinux, and openSUSE targets
	*/vmlinuz-*)
		printf '%s\n' "${kernel##*/vmlinuz-}"
		;;
	# /lib/modules/<release>/vmlinuz, used only if the /boot lookup above
	# did not find a versioned image
	*/vmlinuz)
		release=${kernel%/vmlinuz}
		printf '%s\n' "${release##*/}"
		;;
	*)
		return 1
		;;
	esac
}

# Check that the rootfs contains headers/build files matching the selected
# kernel image, following /lib/modules/<release>/build when needed.
rootfs_has_kernel_headers() {
	local rootfs="$1"
	local kernel_release="$2"
	local build="${rootfs}/lib/modules/${kernel_release}/build"
	local build_target

	# Debian/Ubuntu headers live under /usr/src/linux-headers-<release>
	# Fedora/AlmaLinux headers live under /usr/src/kernels/<release>
	if [ -d "${rootfs}/usr/src/linux-headers-${kernel_release}" ] ||
		[ -d "${rootfs}/usr/src/kernels/${kernel_release}" ] ||
		[ -e "${build}" ]; then
		return 0
	fi

	# openSUSE: if build is an absolute symlink into the rootfs, the
	# host-side -e check above may fail because the target is interpreted
	# relative to the host root
	if [ ! -L "${build}" ]; then
		return 1
	fi

	build_target=$(readlink "${build}")
	case "${build_target}" in
	# absolute symlinks are rooted inside the generated rootfs, not the host
	/*)
		[ -e "${rootfs}${build_target}" ]
		;;
	# relative symlinks are resolved from /lib/modules/<release> (openSUSE
	# should use an absolute symlink but supporting relatives too is cheap)
	*)
		[ -e "$(dirname -- "${build}")/${build_target}" ]
		;;
	esac
}
