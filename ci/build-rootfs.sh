#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

set -euo pipefail

script_dir=$(unset CDPATH; cd -- "$(dirname -- "$0")" && pwd)

# shellcheck source=ci/rootfs-common.sh
. "${script_dir}/rootfs-common.sh"

usage() {
	echo "Usage: $0 <target> <rootfs-dir>" >&2
	exit 1
}

if [ "$#" -ne 2 ]; then
	usage
fi

distro="$1"
rootfs="$2"

if [ -e "${rootfs}" ]; then
	echo "Rootfs path already exists: ${rootfs}" >&2
	exit 1
fi

mkdir -p "$(dirname "${rootfs}")"

build_debian_10() {
	build_debian \
		buster \
		http://archive.debian.org/debian \
		http://archive.debian.org/debian-security \
		buster/updates
}

build_debian_11() {
	build_debian \
		bullseye \
		http://deb.debian.org/debian \
		http://security.debian.org/debian-security \
		bullseye-security
}

build_debian_12() {
	build_debian \
		bookworm \
		http://deb.debian.org/debian \
		http://security.debian.org/debian-security \
		bookworm-security
}

build_debian_13() {
	build_debian \
		trixie \
		http://deb.debian.org/debian \
		http://security.debian.org/debian-security \
		trixie-security
}

build_debian() {
	local codename="$1"
	local mirror="$2"
	local security_mirror="$3"
	local security_suite="$4"
	local debian_keyring="/usr/share/keyrings/debian-archive-keyring.gpg"
	local include
	local mmdebstrap_options=()
	local packages=(
		bc
		binutils
		bison
		build-essential
		ca-certificates
		diffutils
		flex
		git
		iperf3
		iproute2
		iputils-ping
		jq
		kmod
		libelf-dev
		libmbedtls-dev
		libnl-3-dev
		libnl-genl-3-dev
		libssl-dev
		linux-headers-amd64
		linux-image-amd64
		make
		nftables
		pkg-config
		procps
		psmisc
		python3
		python3-jsonschema
		python3-yaml
		rsync
		systemd-sysv
		tcpdump
	)

	if [ "${codename}" = "buster" ]; then
		# Debian 10 is served from archive.debian.org, where old Release files
		# are intentionally expired.
		mmdebstrap_options+=('--aptopt=Acquire::Check-Valid-Until "false"')
	fi

	if [ ! -r "${debian_keyring}" ]; then
		echo "Missing Debian archive keyring: ${debian_keyring}" >&2
		echo "Install debian-archive-keyring on the host." >&2
		exit 1
	fi

	include=$(IFS=,; echo "${packages[*]}")

	# The GitHub runner is Ubuntu, so make the foreign archive trust root
	# explicit instead of depending on the host apt keyring setup.
	mmdebstrap \
		"${mmdebstrap_options[@]}" \
		--variant=minbase \
		--keyring="${debian_keyring}" \
		--include="${include}" \
		"${codename}" \
		"${rootfs}" \
		"deb ${mirror} ${codename} main" \
		"deb ${mirror} ${codename}-updates main" \
		"deb ${security_mirror} ${security_suite} main"
}

build_ubuntu_2004() {
	build_ubuntu focal python3.9
}

build_ubuntu_2204() {
	build_ubuntu jammy
}

build_ubuntu_2404() {
	build_ubuntu noble
}

build_ubuntu_2510() {
	build_ubuntu questing
}

build_ubuntu_2604() {
	build_ubuntu resolute
}

build_ubuntu() {
	local codename="$1"
	local extra_python="${2:-}"
	local include
	local ubuntu_keyring="/usr/share/keyrings/ubuntu-archive-keyring.gpg"
	local packages=(
		bc
		binutils
		bison
		build-essential
		busybox-static
		ca-certificates
		diffutils
		flex
		git
		iperf3
		iproute2
		iputils-ping
		jq
		kmod
		libelf-dev
		libmbedtls-dev
		libnl-3-dev
		libnl-genl-3-dev
		libssl-dev
		linux-headers-generic
		linux-image-generic
		make
		nftables
		pkg-config
		procps
		psmisc
		python3
		python3-jsonschema
		python3-yaml
		rsync
		systemd-sysv
		tcpdump
		udev
	)

	if [ -n "${extra_python}" ]; then
		packages+=("${extra_python}")
	fi

	if [ ! -r "${ubuntu_keyring}" ]; then
		echo "Missing Ubuntu archive keyring: ${ubuntu_keyring}" >&2
		echo "Install ubuntu-keyring on the host." >&2
		exit 1
	fi

	include=$(IFS=,; echo "${packages[*]}")

	# Keep the bootstrap trust root explicit so runner image apt changes do not
	# affect rootfs generation.
	mmdebstrap \
		--variant=minbase \
		--keyring="${ubuntu_keyring}" \
		--include="${include}" \
		"${codename}" \
		"${rootfs}" \
		"deb http://archive.ubuntu.com/ubuntu ${codename} main universe" \
		"deb http://archive.ubuntu.com/ubuntu ${codename}-updates main universe" \
		"deb http://security.ubuntu.com/ubuntu ${codename}-security main universe"
}

mount_dnf_rootfs() {
	mkdir -p \
		"${rootfs}/dev" \
		"${rootfs}/proc" \
		"${rootfs}/sys" \
		"${rootfs}/run"

	# /dev is recursive because package scriptlets may need devices under
	# submounts such as /dev/pts.
	mount --rbind /dev "${rootfs}/dev"
	mount --make-rslave "${rootfs}/dev"
	# procfs and sysfs let kernel package scriptlets query the running host.
	mount -t proc proc "${rootfs}/proc"
	mount -t sysfs sysfs "${rootfs}/sys"
	# Some package scriptlets expect a writable runtime directory.
	mount -t tmpfs tmpfs "${rootfs}/run"
}

umount_dnf_rootfs() {
	umount -R "${rootfs}/run" 2>/dev/null || true
	umount -R "${rootfs}/sys" 2>/dev/null || true
	umount -R "${rootfs}/proc" 2>/dev/null || true
	umount -R "${rootfs}/dev" 2>/dev/null || true
}

build_dnf() {
	local repo_dir="$1"
	local releasever="$2"
	local allow_install_failure="$3"
	local rc clean_rc
	shift 3

	# RPM kernel package scriptlets run inside the installroot and expect
	# procfs, sysfs, devtmpfs, and /run to exist.
	mount_dnf_rootfs

	set +e
	dnf -y \
		--installroot="${rootfs}" \
		--releasever="${releasever}" \
		--setopt="reposdir=${repo_dir}" \
		--setopt=install_weak_deps=False \
		--setopt=tsflags=nodocs \
		install "$@"
	rc=$?
	dnf -y --installroot="${rootfs}" clean all
	clean_rc=$?
	set -e

	umount_dnf_rootfs

	if [ "${rc}" -ne 0 ]; then
		if [ "${allow_install_failure}" = 1 ]; then
			# Some kernel post-install scripts still fail in a minimal chroot
			# after installing the files we need. The rootfs validation below
			# decides whether the result is usable.
			echo "dnf install returned ${rc}; continuing with rootfs validation" >&2
		else
			return "${rc}"
		fi
	fi

	if [ "${clean_rc}" -ne 0 ]; then
		return "${clean_rc}"
	fi
}

build_fedora_44() {
	local repo_dir="${script_dir}/repos/fedora"
	local packages=(
		bc
		binutils
		bison
		ca-certificates
		diffutils
		elfutils-libelf-devel
		flex
		gawk
		gcc
		git
		glibc-devel
		iperf3
		iproute
		iputils
		jq
		kernel
		kernel-devel
		kernel-headers
		kmod
		libnl3-devel
		make
		mbedtls-devel
		nftables
		openssl-devel
		pkgconf-pkg-config
		procps-ng
		psmisc
		python3
		python3-jsonschema
		python3-pyyaml
		rsync
		systemd
		systemd-udev
		tcpdump
	)

	build_dnf "${repo_dir}" 44 0 "${packages[@]}"
}

build_alma() {
	local releasever="$1"
	local repo_dir="${script_dir}/repos/alma"
	local packages=(
		bc
		binutils
		bison
		ca-certificates
		diffutils
		elfutils-libelf-devel
		findutils
		flex
		gawk
		gcc
		git
		glibc-devel
		grep
		iperf3
		iproute
		iputils
		jq
		kernel
		kernel-devel
		kernel-headers
		kmod
		libnl3-devel
		make
		mbedtls-devel
		nftables
		openssl-devel
		pkgconf-pkg-config
		procps-ng
		psmisc
		python3
		python3-jsonschema
		python3-pyyaml
		rsync
		sed
		systemd
		systemd-udev
		tcpdump
	)

	if [ "${releasever}" = 8 ]; then
		repo_dir="${script_dir}/repos/alma8"
	fi

	build_dnf "${repo_dir}" "${releasever}" 1 "${packages[@]}"
}

build_opensuse_leap() {
	local releasever="$1"
	local repo_dir

	# leap 15 has "update-oss" repo but leap 16 doesn't so we have to use
	# different files to avoid 404
	case "${releasever}" in
	15.*)
		repo_dir="${script_dir}/repos/opensuse-leap15"
		;;
	16.*)
		repo_dir="${script_dir}/repos/opensuse-leap16"
		;;
	*)
		echo "Unsupported openSUSE Leap release: ${releasever}" >&2
		exit 1
		;;
	esac

	build_opensuse "${repo_dir}" "${releasever}"
}

build_opensuse_tumbleweed() {
	local repo_dir="${script_dir}/repos/opensuse-tumbleweed"
	build_opensuse "${repo_dir}" tumbleweed
}

build_opensuse() {
	local repo_dir="$1"
	local releasever="$2"
	local packages=(
		bc
		binutils
		bison
		ca-certificates
		diffutils
		findutils
		flex
		gawk
		gcc
		git
		glibc-devel
		grep
		iperf
		iproute2
		iputils
		jq
		kernel-default
		kernel-default-devel
		kernel-devel
		kmod
		libnl3-devel
		make
		mbedtls-devel
		nftables
		openssl-devel
		pkg-config
		procps
		psmisc
		python3
		python3-jsonschema
		python3-PyYAML
		rsync
		sed
		systemd
		tcpdump
		udev
	)

	build_dnf "${repo_dir}" "${releasever}" 1 "${packages[@]}"

	# leap marks non-SUSE modules unsupported unless this policy knob is set
	echo "allow_unsupported_modules 1" \
		> "${rootfs}/etc/modprobe.d/10-unsupported-modules.conf"
}

case "${distro}" in
debian-10)
	build_debian_10
	;;
debian-11)
	build_debian_11
	;;
debian-12)
	build_debian_12
	;;
debian-13)
	build_debian_13
	;;
ubuntu-20.04)
	build_ubuntu_2004
	;;
ubuntu-22.04)
	build_ubuntu_2204
	;;
ubuntu-24.04)
	build_ubuntu_2404
	;;
ubuntu-25.10)
	build_ubuntu_2510
	;;
ubuntu-26.04)
	build_ubuntu_2604
	;;
fedora-44)
	build_fedora_44
	;;
alma-8)
	build_alma 8
	;;
alma-9)
	build_alma 9
	;;
alma-10)
	build_alma 10
	;;
opensuse-leap-15.6)
	build_opensuse_leap 15.6
	;;
opensuse-leap-16.0)
	build_opensuse_leap 16.0
	;;
opensuse-tumbleweed)
	build_opensuse_tumbleweed
	;;
*)
	echo "Unsupported target: ${distro}" >&2
	usage
	;;
esac

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

echo "Built ${distro} rootfs at ${rootfs}"
echo "Kernel: ${kernel_release}"
