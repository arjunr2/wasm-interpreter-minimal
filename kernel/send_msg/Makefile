.PHONY: all clean

#SRC_C := send_msg.c

obj-m += send_msg.o recv_msg.o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement
#send-objs := $(SRC_C:.c=.o)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


