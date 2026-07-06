#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

set -euo pipefail

cd /repo

echo "Guest OS:"
cat /etc/os-release
echo
echo "Guest kernel:"
uname -a
echo
echo "Kernel build directory:"
readlink -f "/lib/modules/$(uname -r)/build"

# The repo is mounted from the host, so root in the guest sees different
# ownership and Git may reject it as a dubious worktree.
git config --global --add safe.directory /repo

make -j"$(nproc)"
make -C tools/testing/selftests TARGETS=net/ovpn clean
make -j"$(nproc)" selftests
make install

# KTAP (the kselftests harness) doesn't provide a "programmatic" way of knowing
# whether a test failed. So we use the same method as NIPA (the netdev CI):
# capture the output and parse for any sign of failure.
selftests_log="$(mktemp)"

# Ubuntu 20.04 ships Python 3.8, but current YNL uses Python 3.9 syntax.
# Prefer the optional Python 3.9 package when that target installed it.
python_path=""
if command -v python3.9 >/dev/null 2>&1 &&
	python3.9 -c 'import jsonschema, yaml' >/dev/null 2>&1; then
	python_path="$(mktemp -d)"
	ln -s "$(command -v python3.9)" "${python_path}/python3"
	export PATH="${python_path}:${PATH}"
fi
trap 'rm -f "$selftests_log"; if [ -n "$python_path" ]; then rm -rf "$python_path"; fi' EXIT

# Preserve make's exit status even though tee is used to keep a log for the
# KTAP failure parser below.
set +e
kselftest_override_timeout="${kselftest_override_timeout:-${KSELFTEST_TIMEOUT:-120}}" \
	OVPN_VERBOSE="${OVPN_VERBOSE:-1}" make run_tests 2>&1 | tee "$selftests_log"
selftests_rc="${PIPESTATUS[0]}"
set -e

if [ "$selftests_rc" -ne 0 ]; then
	exit "$selftests_rc"
fi

if grep -Eq '^[[:space:]#]*not ok [0-9]|# Totals:.*(fail|xpass|error):[1-9]' "$selftests_log"; then
	exit 1
fi
