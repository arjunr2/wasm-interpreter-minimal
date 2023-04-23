.PHONY: all clean

SRC_C := wasmkernel_entry.c src/parse.c src/test.c src/common.c src/ir.c src/opcode_table.c

obj-m += wasmkernel.o
ccflags-y := -std=gnu99 -I$(src)/inc -I$(src) -Wno-declaration-after-statement
wasmkernel-objs := $(SRC_C:.c=.o)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


