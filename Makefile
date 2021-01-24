# SPDX-License-Identifier: GPL-2.0
#
# Makefile - AMD SFH HID drivers
# Copyright (c) 2020-2021, Advanced Micro Devices, Inc.
#
#
ccflags-m := -Werror
obj-$(CONFIG_AMD_SFH_HID) += amd-sfh-pci.o amd-sfh-plat.o
amd-sfh-plat-objs += amd-sfh-hid-ll-drv.o
amd-sfh-plat-objs += amd-sfh-hid-reports.o
amd-sfh-plat-objs += amd-sfh-hid-quirks.o
