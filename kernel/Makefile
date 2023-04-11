.PHONY: all clean

SRC_C := kweerun.c klib.c test.c common.c ir.c

obj-m += weekernel.o
ccflags-y := -std=gnu99 -msoft-float -Wno-declaration-after-statement
weekernel-objs := $(SRC_C:.c=.o)

all:
	echo $(obj-m)
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


