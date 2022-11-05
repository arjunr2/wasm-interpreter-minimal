.PHONY: all clean

all: weerun weeify

clean:
	rm -f weerun *.o

test: weerun
	./weerun -test

weerun: vm.h weerun.c common.h common.c test.h test.c ir.h ir.c weedis.c weedis.h native.h native.c
	cc -o weerun weerun.c common.c test.c ir.c weedis.c native.c -lm

weeify: vm.h weeify.c common.h common.c test.h test.c weewasm.h illegal.h
	cc -o weeify weeify.c common.c

build-test: weerun weeify
	cd tests; ./build.sh
