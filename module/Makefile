obj-m += proc_queue.o
obj-m += proc_sched.o
obj-m += proc_set.o

PWD := $(shell pwd)

KERNELDIR ?= /lib/modules/`uname -r`/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

insmod:
	sudo insmod proc_queue.ko
	sudo insmod proc_sched.ko time_quantum=5
	sudo insmod proc_set.ko
rmmod:
	sudo rmmod proc_set
	sudo rmmod proc_sched
	sudo rmmod proc_queue
