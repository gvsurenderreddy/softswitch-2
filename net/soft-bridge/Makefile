#
# Makefile for the soft switch bridging layer.
#

obj-$(CONFIG_SOFT_BRIDGE) += soft_bridge.o

soft_bridge-y	:= br.o br_device.o br_fdb.o br_forward.o br_if.o br_input.o \
			br_ioctl.o br_notify.o br_stp.o br_stp_bpdu.o \
			br_stp_if.o br_stp_timer.o br_netlink.o

#obj-m += ram_bridge_log.o
obj-m += br_new_mtu.o

soft_bridge-$(CONFIG_SYSFS) += br_sysfs_if.o br_sysfs_br.o

soft_bridge-$(CONFIG_BRIDGE_NETFILTER) += br_netfilter.o

obj-$(CONFIG_SOFT_BRIDGE_NF_EBTABLES) += netfilter/
