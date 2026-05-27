#!/bin/bash
#
# This script automates the creation of compatibility patch files by comparing
# original kernel files with modified versions. It separates the patches into
# two categories:
# - source Patches: for the actual source code files
# - test Patches: for the selftests files
#
# Default directories:
#   - original sources: kernel/drivers/net/ovpn
#   - original tests:   kernel/tools/testing/selftests/net/ovpn
#   - modified sources: drivers/net/ovpn
#   - modified tests:   tools/testing/selftests/net/ovpn
#
# Usage:
#   Ensure that the modified source and test files exist in drivers/net/ovpn
#   and tools/testing/selftests/net/ovpn respectively relative to your
#   repository root. Then override ORIG_SOURCES_DIR and ORIG_TESTS_DIR if
#   needed and run the script.
#   It creates patch files in compat-patches/sources and compat-patches/tests
#   using 'git diff --no-index' (empty patches are removed) and substitutes
#   original paths with modified ones.

ORIG_SOURCES_DIR=${ORIG_SOURCES_DIR:-kernel/drivers/net/ovpn}
ORIG_TESTS_DIR=${ORIG_TESTS_DIR:-kernel/tools/testing/selftests/net/ovpn}
MOD_SOURCES_DIR="drivers/net/ovpn"
MOD_TESTS_DIR="tools/testing/selftests/net/ovpn"

# create the escaped version of the directories
if [[ $ORIG_SOURCES_DIR =~ ^/ ]]; then
	substr="${ORIG_SOURCES_DIR:1}"
	ESC_ORIG_SOURCES_DIR="${substr//\//\\\/}"
else
	ESC_ORIG_SOURCES_DIR="${ORIG_SOURCES_DIR//\//\\\/}"
fi

if [[ $ORIG_TESTS_DIR =~ ^/ ]]; then
	substr="${ORIG_TESTS_DIR:1}"
	ESC_ORIG_TESTS_DIR="${substr//\//\\\/}"
else
	ESC_ORIG_TESTS_DIR="${ORIG_TESTS_DIR//\//\\\/}"
fi

ESC_MOD_SOURCES_DIR="${MOD_SOURCES_DIR//\//\\\/}"
ESC_MOD_TESTS_DIR="${MOD_TESTS_DIR//\//\\\/}"

is_kernel_build_artifact() {
	case "$1" in
		*.ko|*.mod|*.mod.c|*.o|Module.symvers|modules.order)
			return 0
			;;
	esac

	return 1
}

# remove old patches
rm -f compat-patches/sources/*.patch
rm -f compat-patches/tests/*.patch

# create the directories if they don't exist
mkdir -p compat-patches/sources
mkdir -p compat-patches/tests

# handle the source files
for filepath in "$ORIG_SOURCES_DIR"/*; do
    base=$(basename "$filepath")
    out="compat-patches/sources/${base}.patch"
    if [[ -f "$filepath" && -f "$MOD_SOURCES_DIR/$base" ]]; then
        git diff --no-index "$filepath" "$MOD_SOURCES_DIR/$base" > "$out"
        # if the patch is empty, remove it
        if [ ! -s "$out" ]; then
            rm -f "$out"
            continue
        fi
        sed -i "s/$ESC_ORIG_SOURCES_DIR/$ESC_MOD_SOURCES_DIR/" "$out"
    fi
done
for filepath in "$MOD_SOURCES_DIR"/*; do
    base=$(basename "$filepath")
    out="compat-patches/sources/${base}.patch"
    if [[ -f "$filepath" && ! -e "$ORIG_SOURCES_DIR/$base" ]]; then
        if is_kernel_build_artifact "$base"; then
            continue
        fi
        if git diff --no-index --numstat /dev/null "$filepath" | \
            grep -q '^-[[:space:]]\+-[[:space:]]'; then
            continue
        fi
        git diff --no-index /dev/null "$filepath" > "$out"
    fi
done

# handle the test files
for filepath in "$ORIG_TESTS_DIR"/*; do
    base=$(basename "$filepath")
    out="compat-patches/tests/${base}.patch"
    if [[ -f "$filepath" && -f "$MOD_TESTS_DIR/$base" ]]; then
        git diff --no-index "$filepath" "$MOD_TESTS_DIR/$base" > "$out"
        # if the patch is empty, remove it
        if [ ! -s "$out" ]; then
            rm -f "$out"
            continue
        fi
        sed -i "s/$ESC_ORIG_TESTS_DIR/$ESC_MOD_TESTS_DIR/" "$out"
    fi
done
for filepath in "$MOD_TESTS_DIR"/*; do
    base=$(basename "$filepath")
    out="compat-patches/tests/${base}.patch"
    if [[ -f "$filepath" && ! -e "$ORIG_TESTS_DIR/$base" ]]; then
        if git diff --no-index --numstat /dev/null "$filepath" | \
            grep -q '^-[[:space:]]\+-[[:space:]]'; then
            continue
        fi
        git diff --no-index /dev/null "$filepath" > "$out"
    fi
done
