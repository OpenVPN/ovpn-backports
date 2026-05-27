# Out-of-tree backports for the ovpn kernel module

The ovpn kernel module is part of the Linux kernel starting from version 6.16. It enhances OpenVPN performance by offloading data channel management to the kernel. This repository contains out-of-tree backports for the ovpn kernel module, enabling its use with older kernel versions.

## Branch structure

The repository consists of two main branches:
- `main`: tracks the latest code from the `net-next` tree.
- `net`: tracks the latest code from the `net` tree.

Additionally, there are two branches dedicated to development and testing, which may contain code not yet part of any official kernel tree:
- `development`: code built on top of the `net-next` tree.
- `development-net`: code built on top of the `net` tree.

Finally, there are four corresponding branches for pre-patched sources:
- `sources`: pre-patched sources for `main`.
- `net-sources`: pre-patched sources for `net`.
- `development-sources`: pre-patched sources for `development`.
- `development-net-sources`: pre-patched sources for `development-net`.

## Installation

### Packages

You can download packages from the [OpenVPN OBS repository](https://download.opensuse.org/repositories/isv:/OpenVPN:/Snapshots/).

### Pre-patched sources

#### Tags

A pre-patched version of the source files is available for download in the [tags](https://github.com/OpenVPN/ovpn-backports/tags) section of this repository. Generally, you’ll want to download the packages containing either 'main' or 'net' in the name, as these track the 'net-next' and 'net' trees, respectively (see [branch structure](#branch-structure)).

After downloading and ensuring the kernel headers are installed, simply run `make && make install` to build and install the module.

#### Sources branch

Another way to download the pre-patched sources is to clone the repository and check out a branch containing `sources` in the name (see [branch structure](#branch-structure)). Then, after ensuring the kernel headers are installed, simply run `make && make install` to build and install the module.

### Building from source

#### Preparing the build environment

To build the module, you need the kernel headers (or sources) and the ovpn source files. You can either compile the module against your currently running kernel (by ensuring the headers are installed) or against a different kernel version by providing the appropriate path to the kernel sources.

The first step is to get the latest ovpn sources through the utility script `backports-ctl.sh`:

```sh
./backports-ctl.sh get-ovpn
```

Be aware that this command temporarily downloads a full kernel tree, extracts the ovpn files from it, and applies patches. If you want the kernel repository to persist, run the command with the `--keep` (or `-k`) option. This option keeps the repository in the `kernel` directory as a shallow copy, and subsequent runs will simply update it.

To import the ovpn selftests as well, add the `--tests` (or `-t`) option.

Additionally you can restore the repository to its original state with:

```sh
./backports-ctl.sh clean
```

#### Building and installing

To build the ovpn kernel module, just type:

```sh
make
```

in the root folder. The Makefile will autodetect your running kernel and will try to use its headers to get the code compiled.

To include debugging symbols in the module, you can run:

```sh
make debug # or make DEBUG=1
```

If you want to build ovpn against a kernel different from the one running on the host, run:

```sh
make KERNEL_SRC=/path/to/the/kernel/tree
```

The control is passed to the kernel Makefile, therefore any kernel Makefile argument can be specified on the command line and it will be passed automatically. Once done building, executing the command:

```sh
make install
```

will install the ovpn.ko kernel module in the updates/ subfolder of the kernel modules directory on your system. It normally means `/lib/modules/$(uname -r)/updates/`.

> **_NOTE:_** If Secure Boot is enabled, you need to sign the module before loading it. Check [this](https://askubuntu.com/questions/760671/could-not-load-vboxdrv-after-upgrade-to-ubuntu-16-04-and-i-want-to-keep-secur/768310#768310) tutorial on how to sign a custom kernel module.

## Testing

If the selftests were imported with `backports-ctl.sh get-ovpn -t`, you can build them with:

```sh
make selftests
```

After building and installing the module, run the ovpn selftests with:

```sh
make run_tests
```

### Kernel versions

The module was compiled and tested on the following distributions:
 - Ubuntu 20.04, 22.04, 24.04
 - Debian 11, 12
 - RHEL 8, 9, 10
 - openSUSE Tumbleweed/Slowroll
 - openSUSE SLE 15
