#!/bin/bash

set -e

# remember whether the caller overrode the pin before loading its default
KERNEL_COMMIT_OVERRIDDEN=${KERNEL_COMMIT+x}
BACKPORTS_CONFIG="$PWD/backports.conf"
# shellcheck source=backports.conf
source "$BACKPORTS_CONFIG"
KERNEL_DIR="$PWD/kernel"
UPDATE_STATE="$PWD/.backports-update"
ORIG_SOURCES_DIR=${ORIG_SOURCES_DIR:-kernel/drivers/net/ovpn}
ORIG_TESTS_DIR=${ORIG_TESTS_DIR:-kernel/tools/testing/selftests/net/ovpn}
ORIG_YNL_DIR=${ORIG_YNL_DIR:-kernel/tools/net/ynl/pyynl}
MOD_SOURCES_DIR=drivers/net/ovpn
MOD_TESTS_DIR=tools/testing/selftests/net/ovpn
MOD_YNL_DIR=tools/net/ynl/pyynl
COMPAT_PATCHES=()

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

extract_ovpn_sources() {
	local source_tree=$1

	echo "Extracting ovpn source files"
	rm -fr "$PWD/drivers/" "$PWD/include/"
	mkdir -p "$PWD/drivers/net" "$PWD/include/uapi/linux"
	cp -r "$source_tree/drivers/net/ovpn" "$PWD/drivers/net/"
	cp "$source_tree/include/uapi/linux/ovpn.h" "$PWD/include/uapi/linux/ovpn.h"
}

import_ovpn_selftests() {
	local source_tree=${1:-$KERNEL_DIR}

	echo "Extracting ovpn selftests"
	clean_ovpn_selftests
	mkdir -p \
		"$PWD/tools/testing/selftests/net" \
		"$PWD/tools/net/ynl" \
		"$PWD/Documentation/netlink/specs" \
		"$PWD/scripts"

	cp -r "$source_tree/tools/testing/selftests/net/ovpn" \
		"$PWD/tools/testing/selftests/net/"
	cp -r "$source_tree/tools/testing/selftests/kselftest" \
		"$PWD/tools/testing/selftests/"
	cp "$source_tree/tools/testing/selftests/Makefile" \
		"$PWD/tools/testing/selftests/Makefile"
	cp "$source_tree/tools/testing/selftests/lib.mk" \
		"$PWD/tools/testing/selftests/lib.mk"
	cp "$source_tree/tools/testing/selftests/kselftest.h" \
		"$PWD/tools/testing/selftests/kselftest.h"
	cp "$source_tree/tools/testing/selftests/kselftest_harness.h" \
		"$PWD/tools/testing/selftests/kselftest_harness.h"
	cp "$source_tree/tools/testing/selftests/run_kselftest.sh" \
		"$PWD/tools/testing/selftests/run_kselftest.sh"
	cp -r "$source_tree/tools/net/ynl/pyynl" "$PWD/tools/net/ynl/"
	cp "$source_tree/Documentation/netlink/specs/ovpn.yaml" \
		"$PWD/Documentation/netlink/specs/ovpn.yaml"
	cp "$source_tree/Documentation/netlink/genetlink.yaml" \
		"$PWD/Documentation/netlink/genetlink.yaml"
	cp "$source_tree/scripts/subarch.include" "$PWD/scripts/subarch.include"

}

collect_compat_patches() {
	local import_tests=$1
	local patch_root=${2:-$PWD/compat-patches}
	local patch

	COMPAT_PATCHES=()

	# without nullglob, an unmatched pattern is passed through literally
	for patch in "$patch_root"/sources/*.patch; do
		[ -e "$patch" ] || continue
		COMPAT_PATCHES+=("$patch")
	done

	if [ "$import_tests" -eq "1" ]; then
		for patch in "$patch_root"/tests/*.patch \
			"$patch_root"/ynl/*.patch; do
			[ -e "$patch" ] || continue
			COMPAT_PATCHES+=("$patch")
		done
	fi
}

apply_compat_patches() {
	local import_tests=$1

	collect_compat_patches "$import_tests"

	if [ "${#COMPAT_PATCHES[@]}" -gt "0" ]; then
		# a single invocation validates the entire series before
		# modifying the files
		git apply --verbose "${COMPAT_PATCHES[@]}"
	fi
}

is_kernel_build_artifact() {
	case "$1" in
	*.ko | *.mod | *.mod.c | *.o | Module.symvers | modules.order)
		return 0
		;;
	esac

	return 1
}

write_diff() {
	local old_file=$1
	local new_file=$2
	local output=$3
	local status=0

	# exit status 1 means a diff was produced; only propagate actual errors
	git diff --no-index "$old_file" "$new_file" > "$output" || status=$?
	if [ "$status" -ne "1" ]; then
		return "$status"
	fi
}

generate_patch_set() (
	local orig_dir=$1
	local mod_dir=$2
	local output_dir=$3
	local target_dir=${4:-$mod_dir}
	local escaped_orig_dir=${orig_dir#/}
	local escaped_mod_dir=${mod_dir#/}
	local escaped_target_dir=${target_dir#/}
	local base filepath output patch_name relative_path

	escaped_orig_dir=${escaped_orig_dir//\//\\/}
	escaped_mod_dir=${escaped_mod_dir//\//\\/}
	escaped_target_dir=${escaped_target_dir//\//\\/}
	mkdir -p "$output_dir"

	# Recurse into trees such as pyynl/lib, and make an empty tree expand
	# to nothing instead of a literal **/* path. The subshell scopes both
	# options.
	shopt -s globstar nullglob

	for filepath in "$orig_dir"/**/*; do
		[ -f "$filepath" ] || continue
		relative_path=${filepath#"$orig_dir"/}
		# patch dirs are flat, so encode subdirs in the name
		patch_name=${relative_path//\//-}.patch
		output="$output_dir/$patch_name"

		if [ -f "$mod_dir/$relative_path" ]; then
			write_diff "$filepath" "$mod_dir/$relative_path" "$output"
			if [ ! -s "$output" ]; then
				rm -f "$output"
				continue
			fi
			sed -i \
				-e "s/$escaped_orig_dir/$escaped_target_dir/" \
				-e "s/$escaped_mod_dir/$escaped_target_dir/" "$output"
		fi
	done

	for filepath in "$mod_dir"/**/*; do
		[ -f "$filepath" ] || continue
		relative_path=${filepath#"$mod_dir"/}
		base=$(basename "$filepath")
		# catch Kbuild output files
		if [ -e "$orig_dir/$relative_path" ] || is_kernel_build_artifact "$base"; then
			continue
		fi
		# catch bin files (such as .pyc) reported as "- -" by numstat
		if git diff --no-index --numstat /dev/null "$filepath" | \
			grep -q '^-[[:space:]]\+-[[:space:]]'; then
			continue
		fi

		patch_name=${relative_path//\//-}.patch
		output="$output_dir/$patch_name"
		write_diff /dev/null "$filepath" "$output"
		sed -i "s/$escaped_mod_dir/$escaped_target_dir/" "$output"
	done
)

replace_patch_set() {
	local generated_dir=$1
	local patch_dir=$2
	local patch

	mkdir -p "$patch_dir"
	rm -f "$patch_dir"/*.patch
	# without nullglob, an unmatched pattern is passed through literally
	for patch in "$generated_dir"/*.patch; do
		[ -e "$patch" ] || continue
		cp "$patch" "$patch_dir/"
	done
}

check_resolved_sources() {
	local path
	local unresolved=0

	for path in "$MOD_SOURCES_DIR" "$MOD_TESTS_DIR" "$MOD_YNL_DIR"; do
		[ -d "$path" ] || continue
		if find "$path" -type f -name '*.rej' -print | grep -q .; then
			echo "Unresolved reject files found under $path" >&2
			unresolved=1
		fi
		if grep -RIlE '^(<<<<<<< |=======|>>>>>>> )' "$path" | grep -q .; then
			echo "Unresolved conflict markers found under $path" >&2
			unresolved=1
		fi
	done

	[ "$unresolved" -eq "0" ]
}

refresh_patches() (
	local tmp_dir

	if [ ! -d "$ORIG_SOURCES_DIR" ] || [ ! -d "$MOD_SOURCES_DIR" ]; then
		echo "Original and modified ovpn sources are required" >&2
		return 1
	fi
	check_resolved_sources

	tmp_dir=$(mktemp -d "$PWD/.compat-patches.XXXXXX")
	trap 'rm -rf "$tmp_dir"' EXIT

	generate_patch_set "$ORIG_SOURCES_DIR" "$MOD_SOURCES_DIR" \
		"$tmp_dir/sources"
	if [ -d "$MOD_TESTS_DIR" ]; then
		generate_patch_set "$ORIG_TESTS_DIR" "$MOD_TESTS_DIR" \
			"$tmp_dir/tests"
	fi
	if [ -d "$MOD_YNL_DIR" ]; then
		generate_patch_set "$ORIG_YNL_DIR" "$MOD_YNL_DIR" \
			"$tmp_dir/ynl"
	fi

	replace_patch_set "$tmp_dir/sources" compat-patches/sources
	if [ -d "$MOD_TESTS_DIR" ]; then
		replace_patch_set "$tmp_dir/tests" compat-patches/tests
	fi
	if [ -d "$MOD_YNL_DIR" ]; then
		replace_patch_set "$tmp_dir/ynl" compat-patches/ynl
	fi
)

ensure_kernel_repo() {
	if [ ! -d "$KERNEL_DIR" ]; then
		git clone --depth 1 "$KERNEL_REPO_URL" "$KERNEL_DIR"
	fi
}

fetch_kernel_commit() {
	local commit=$1

	if ! git -C "$KERNEL_DIR" cat-file -e "$commit^{commit}" 2>/dev/null; then
		git -C "$KERNEL_DIR" fetch --depth 1 origin "$commit"
	fi
}

write_version_info() {
	local branch tree kernel_version backports_commit

	# We extract the branch from backports because all the non-sources
	# branches of this repo point directly to the corresponding branch in
	# ovpn-net-next and there's no way of getting this info directly from
	# ovpn-net-next since it has been cloned with --depth 1.
	branch=$(git rev-parse --abbrev-ref HEAD)

	# save version information to a file (as key=value pairs) unless we're
	# in a sources branch
	if [[ $branch == *'sources'* ]]; then
		return
	fi

	echo "Setting version information"

	# name of the repository from where the ovpn sources were extracted
	tree=$(basename "$(git -C "$KERNEL_DIR" config --get remote.origin.url)" | cut -d. -f1)

	# version of the kernel from where the ovpn sources were extracted
	kernel_version=$(make -s -C "$KERNEL_DIR" kernelversion)

	# this indirectly indicates also the ovpn-net-next commit used for
	# generating the backports
	backports_commit=$(git rev-parse --short HEAD)

	rm -f "$PWD/.version"
	cat << EOF > "$PWD/.version"
tree=${tree}
branch=${branch}
kernel_version=${kernel_version}
backports_commit=${backports_commit}
EOF
}

create_kernel_worktree() {
	local commit=$1
	local worktree

	worktree=$(mktemp -d /tmp/ovpn-backports-update.XXXXXX)
	rmdir "$worktree"
	git -C "$KERNEL_DIR" worktree add --detach "$worktree" "$commit" >&2
	printf '%s\n' "$worktree"
}

remove_kernel_worktree() {
	git -C "$KERNEL_DIR" worktree remove --force "$1"
}

write_update_state() {
	local old_commit=$1
	local new_commit=$2

	mkdir "$UPDATE_STATE"
	cp -r "$PWD/compat-patches" "$UPDATE_STATE/compat-patches"
	cat << EOF > "$UPDATE_STATE/state"
UPDATE_OLD_COMMIT=$old_commit
UPDATE_NEW_COMMIT=$new_commit
EOF
}

load_update_state() {
	if [ ! -f "$UPDATE_STATE/state" ]; then
		echo "No ovpn update is in progress" >&2
		return 1
	fi

	# shellcheck source=/dev/null
	source "$UPDATE_STATE/state"
}

prepare_generated_tree() {
	local source_tree=$1

	clean_ovpn_selftests
	extract_ovpn_sources "$source_tree"
	import_ovpn_selftests "$source_tree"
}

verify_patch_series() {
	local commit=$1
	local status=0
	local worktree

	worktree=$(create_kernel_worktree "$commit")
	collect_compat_patches 1
	git -C "$worktree" apply --check "${COMPAT_PATCHES[@]}" || status=$?
	remove_kernel_worktree "$worktree"
	return "$status"
}

normalize_patch_series() {
	local old_commit=$1
	local output_dir=$2
	local status=0
	local worktree

	# re-diff the applied series against the configured base so every patch
	# has current blob IDs and context before the three-way update
	git -C "$KERNEL_DIR" reset --hard "$old_commit"
	worktree=$(create_kernel_worktree "$old_commit")
	collect_compat_patches 1
	git -C "$worktree" apply "${COMPAT_PATCHES[@]}" || status=$?
	if [ "$status" -ne "0" ]; then
		remove_kernel_worktree "$worktree"
		return "$status"
	fi

	generate_patch_set "$KERNEL_DIR/drivers/net/ovpn" \
		"$worktree/drivers/net/ovpn" "$output_dir/sources" \
		"$MOD_SOURCES_DIR"
	generate_patch_set "$KERNEL_DIR/tools/testing/selftests/net/ovpn" \
		"$worktree/tools/testing/selftests/net/ovpn" "$output_dir/tests" \
		"$MOD_TESTS_DIR"
	generate_patch_set "$KERNEL_DIR/tools/net/ynl/pyynl" \
		"$worktree/tools/net/ynl/pyynl" "$output_dir/ynl" \
		"$MOD_YNL_DIR"
	remove_kernel_worktree "$worktree"
}

update_config_commit() {
	local new_commit=$1
	local tmp_config

	tmp_config=$(mktemp "$PWD/.backports.conf.XXXXXX")
	sed "s|^KERNEL_COMMIT=.*|KERNEL_COMMIT=\${KERNEL_COMMIT:-'$new_commit'}|" \
		"$BACKPORTS_CONFIG" > "$tmp_config"
	chmod 0644 "$tmp_config"
	mv "$tmp_config" "$BACKPORTS_CONFIG"
}

finish_ovpn_update() {
	local status=0

	load_update_state
	check_resolved_sources
	ensure_kernel_repo
	fetch_kernel_commit "$UPDATE_NEW_COMMIT"
	git -C "$KERNEL_DIR" reset --hard "$UPDATE_NEW_COMMIT"

	refresh_patches
	verify_patch_series "$UPDATE_NEW_COMMIT" || status=$?
	if [ "$status" -ne "0" ]; then
		echo "Regenerated patches do not apply to $UPDATE_NEW_COMMIT" >&2
		return "$status"
	fi

	write_version_info
	update_config_commit "$UPDATE_NEW_COMMIT"
	rm -rf "$UPDATE_STATE"
	echo "Updated ovpn-net-next to $UPDATE_NEW_COMMIT"
}

start_ovpn_update() {
	local requested_commit=$1
	local apply_status=0
	local conflicts new_commit old_commit worktree

	if [ -n "$KERNEL_COMMIT_OVERRIDDEN" ]; then
		echo "update-ovpn cannot be used with a KERNEL_COMMIT override" >&2
		return 1
	fi
	if [ -e "$UPDATE_STATE" ]; then
		echo "An ovpn update is already in progress" >&2
		return 1
	fi

	ensure_kernel_repo
	fetch_kernel_commit "$KERNEL_COMMIT"
	fetch_kernel_commit "$requested_commit"
	old_commit=$(git -C "$KERNEL_DIR" rev-parse "$KERNEL_COMMIT^{commit}")
	new_commit=$(git -C "$KERNEL_DIR" rev-parse "$requested_commit^{commit}")

	write_update_state "$old_commit" "$new_commit"
	echo "Normalizing patches against $old_commit"
	normalize_patch_series "$old_commit" "$UPDATE_STATE/normalized-patches"

	git -C "$KERNEL_DIR" reset --hard "$new_commit"
	worktree=$(create_kernel_worktree "$new_commit")
	collect_compat_patches 1 "$UPDATE_STATE/normalized-patches"
	git -C "$worktree" apply --3way "${COMPAT_PATCHES[@]}" || apply_status=$?
	conflicts=$(git -C "$worktree" diff --name-only --diff-filter=U)
	prepare_generated_tree "$worktree"
	remove_kernel_worktree "$worktree"

	if [ -n "$conflicts" ]; then
		echo "Compatibility patches need conflict resolution:" >&2
		while IFS= read -r conflict; do
			printf '  %s\n' "$conflict" >&2
		done <<< "$conflicts"
		echo "Resolve them in the generated tree, then run:" >&2
		echo "  ./backports-ctl.sh update-ovpn --continue" >&2
		return 1
	fi
	if [ "$apply_status" -ne "0" ]; then
		echo "Compatibility patches could not be applied to $new_commit" >&2
		echo "Run './backports-ctl.sh update-ovpn --abort' to restore the old tree" >&2
		return "$apply_status"
	fi

	finish_ovpn_update
}

abort_ovpn_update() {
	load_update_state
	replace_patch_set "$UPDATE_STATE/compat-patches/sources" compat-patches/sources
	replace_patch_set "$UPDATE_STATE/compat-patches/tests" compat-patches/tests
	replace_patch_set "$UPDATE_STATE/compat-patches/ynl" compat-patches/ynl
	KERNEL_COMMIT=$UPDATE_OLD_COMMIT
	get_ovpn 1 1
	rm -rf "$UPDATE_STATE"
	echo "Aborted ovpn update"
}

get_ovpn() {
	local keep=$1
	local import_tests=$2

	ensure_kernel_repo
	fetch_kernel_commit "$KERNEL_COMMIT"

	echo "Checking out commit $KERNEL_COMMIT"
	git -C "$KERNEL_DIR" reset --hard "$KERNEL_COMMIT"

	clean_ovpn_selftests
	extract_ovpn_sources "$KERNEL_DIR"

	if [ "$import_tests" -eq "1" ]; then
		import_ovpn_selftests
	fi

	apply_compat_patches "$import_tests"
	write_version_info

	if [ "$keep" -eq "0" ] ; then
		echo "Cleaning up"
		rm -rf "$KERNEL_DIR"
	fi
}

print_usage() {
	echo "Usage: ./backports-ctl.sh <get-ovpn|refresh-patches|update-ovpn|clean>"
	echo "       ./backports-ctl.sh get-ovpn [-k|--keep] [-t|--tests]"
	echo "       ./backports-ctl.sh update-ovpn <new-commit|--continue|--abort>"
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
elif [ "$command" = "refresh-patches" ]; then
	refresh_patches
elif [ "$command" = "update-ovpn" ]; then
	shift
	if [ "$#" -ne "1" ]; then
		print_usage
	fi
	case "$1" in
	--continue)
		finish_ovpn_update
		;;
	--abort)
		abort_ovpn_update
		;;
	-*)
		print_usage
		;;
	*)
		start_ovpn_update "$1"
		;;
	esac
elif [ "$command" = "clean" ]; then
	read -r -p "Are you sure you want to restore the repository to its default state? [y/N]" clean && [[ "$clean" = "y" || "$clean" = "Y" ]] && git clean -fdx && git reset --hard
else
	echo "Unknown command $command"
	print_usage
fi
