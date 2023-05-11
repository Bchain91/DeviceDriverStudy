MOD := hello
obj-m := $(MOD).o

#CROSS = ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
#KDIR := /lib/modules/$(shell uname -r)/build
KDIR := /home/udooer/kernel/linux_kernel-3.14-1.0.x-udoo 
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules $(CROSS)
#cp $(MOD).ko /srv/nfs
clean:
#	rm -rf *.ko
	rm -rf *.mod.*
	rm -rf .*.cmd
	rm -rf *.o
	rm -rf modules.order
	rm -rf Module.symvers
	rm -rf $(MOD).mod
	rm -rf .tmp_versions
