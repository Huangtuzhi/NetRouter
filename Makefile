obj-m:=ed_device.o
CURRENT_PATH:=$(shell pwd)
ARM_LINUX_KERNEL:=/opt/linux-2.6.32.2
all:
	$(MAKE) -C $(ARM_LINUX_KERNEL) SUBDIRS=$(CURRENT_PATH)  -m
clean:
	rm -rf *.cmd *.o *.ko
