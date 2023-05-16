MOD := kerneltimer
obj-m := $(MOD).o
PWD := $(shell pwd)

#KDIR := /lib/modules/$(shell uname -r)/build
KDIR := /home/udooer/kernel/linux_kernel-3.14-1.0.x-udoo
ARCH = ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules $(ARCH)
#	cp $(MOD).ko /srv/nfs

clean:
	rm -rf *.ko
	rm -rf *.o
	rm -rf .*.cmd
	rm -rf .tmp_*
	rm -rf *.mod.*
	rm -rf Module.*
	rm -rf modules.*
