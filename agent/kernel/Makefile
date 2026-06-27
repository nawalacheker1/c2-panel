obj-m += c2_kernel.o

c2_kernel-objs := c2_kernel.o

EXTRA_CFLAGS += -Wall -O2

KERNEL_VERSION := $(shell uname -r)

all:
	make -C /lib/modules/$(KERNEL_VERSION)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(KERNEL_VERSION)/build M=$(PWD) clean

load:
	sudo insmod c2_kernel.ko

remove:
	sudo rmmod c2_kernel

test:
	sudo dmesg | grep c2 | tail -30
