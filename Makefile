MAKEFLAGS += -rR --no-print-directory

unexport LC_ALL
LC_COLLATE=C
LC_NUMERIC=C
export LC_COLLATE LC_NUMERIC

V =
ifeq ($(strip $(V)),)
	E = @echo
	Q = @
else
	E = @\#
	Q =
endif
export E Q

ifeq ("$(origin V)", "command line")
	KBUILD_VERBOSE = $(V)
endif
ifndef KBUILD_VERBOSE
	KBUILD_VERBOSE = 0
endif

ifeq ("$(origin C)", "command line")
	KBUILD_CHECKSRC = $(C)
endif
ifndef KBUILD_CHECKSRC
	KBUILD_CHECKSRC = 0
endif

ifeq ("$(origin M)", "command line")
	KBUILD_EXTMOD := $(M)
endif

obj-m := kbrainfuck.o 

KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

RES = .kbrainfuck.ko.cmd .kbrainfuck.mod.o.cmd .kbrainfuck.o.cmd Module.symvers \
 kbrainfuck.ko kbrainfuck.mod.c kbrainfuck.mod.o kbrainfuck.o modules.order

DIR = .tmp_versions/

kbrainfuck: kbrainfuck.c
	$(E) "  CC     " $@
	$(Q) $(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(E) "  CLEAN   " $(RES)
	$(Q) rm -f $(RES)
	$(E) "  CLEAN   " $(DIR)
	$(Q) rm -rf $(DIR)
