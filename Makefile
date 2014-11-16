obj-m:=ed_device.o
KERNELDIR:=/lib/modules/3.0.0-14-generic/build
PWD:=$(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules


