.PHONY: all clean

SRC_C := kweerun.c src/klib.c src/test.c src/common.c src/ir.c

obj-m += weekernel.o
ccflags-y := -std=gnu99 -I$(src)/inc -I$(src) -Wno-declaration-after-statement
weekernel-objs := $(SRC_C:.c=.o)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


