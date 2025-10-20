PWD:=$(shell pwd)
KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
KERNEL_CFG ?= $(KERNEL_SRC)/.config
OVPN_CFG = $(PWD)/ovpn.config

ifeq ($(shell cd $(KERNEL_SRC) && pwd),)
$(error $(KERNEL_SRC) is missing, please set KERNEL_SRC)
endif

ifeq ("$(wildcard $(KERNEL_CFG))","")
$(error $(KERNEL_CFG) is missing, please set KERNEL_CFG)
endif

export KERNEL_SRC
RM ?= rm -f
CP := cp -fpR
LN := ln -sf
DEPMOD := depmod -a

ifneq ("$(wildcard $(KERNEL_SRC)/include/generated/uapi/linux/suse_version.h)","")
VERSION_INCLUDE = -include linux/suse_version.h
endif

DEBUG ?= 0
ifeq ($(DEBUG),1)
    ccflags-y += -g -DDEBUG
endif

# extract values from .version and construct the module version string
ifneq ("$(wildcard $(PWD)/.version)","")
    tree             := $(shell . $(PWD)/.version && echo $$tree)
    branch           := $(shell . $(PWD)/.version && echo $$branch)
    kernel_version   := $(shell . $(PWD)/.version && echo $$kernel_version)
    backports_commit := $(shell . $(PWD)/.version && echo $$backports_commit)
    version_str      := $(tree)/$(branch)-$(kernel_version)-$(backports_commit)
    ccflags-y        += -DOVPN_MODULE_VERSION=\"$(version_str)\"
else
    $(warning '.version' file not found — have you run 'backports-ctl.sh get-ovpn'?)
endif

NOSTDINC_FLAGS += \
	-I$(PWD)/compat-include/ \
	-I$(PWD)/include/ \
	-include $(PWD)/linux-compat.h \
	$(ccflags-y)

obj-y += drivers/net/ovpn/
export ovpn

BUILD_FLAGS := \
	M=$(PWD) \
	PWD=$(PWD) \
	CONFIG_OVPN=m \
	INSTALL_MOD_DIR=updates/

all: check-config
	$(MAKE) -C $(KERNEL_SRC) $(BUILD_FLAGS) modules

debug:
	$(MAKE) DEBUG=1 all

check-config:
	@while read -r flag; do \
		if [ -z "$$flag" ] || [ "$${flag#\#}" != "$$flag" ]; then continue; fi; \
		if ! grep -qE "^$$flag=(y|m)" $(KERNEL_CFG); then \
			echo "Error: Kernel configuration flag '$$flag' is not set to 'y' or 'm' in $(KERNEL_CFG)" >&2; \
			exit 1; \
		fi; \
	done < $(OVPN_CFG)
	@echo "Kernel configuration check passed."

clean:
	$(MAKE) -C $(KERNEL_SRC) $(BUILD_FLAGS) clean
	$(MAKE) -C $(PWD)/tests/ovpn-cli $(BUILD_FLAGS) clean

install:
	$(MAKE) -C $(KERNEL_SRC) $(BUILD_FLAGS) modules_install
	$(DEPMOD)

ovpn-cli:
	$(MAKE) -C $(PWD)/tests/ovpn-cli $(BUILD_FLAGS) ovpn-cli

.PHONY: all debug check-config clean install ovpn-cli
