MAKEFLAGS += -rR --no-print-directory

obj-m := kbrainfuck.o

KDIR = /lib/modules/$(shell uname -r)/build

RES = .*.cmd *.mod.c *.o *.ko Module.symvers modules.order 
DIR = .tmp_versions/

kbrainfuck:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -f $(RES)
	rm -rf $(DIR)

load:	kbrainfuck
	insmod ./kbrainfuck.ko

unload:
	rmmod kbrainfuck

reload: unload load
