.PHONY: all clean

CC := gcc
CFLAGS = -g -O3 -Iinc -I.

BUILD_DIR := build
SRC_C := src/parse.c src/common.c \
					src/ir.c src/opcode_table.c src/instantiate.c src/run.c

SRC_O := $(addprefix $(BUILD_DIR)/, $(SRC_C:src/%.c=%.o) wasmkernel_user.o)


all: dir wasmkernel
	echo $(SRC_O)

dir:
	mkdir -p $(BUILD_DIR)

wasmkernel: $(SRC_O)
	$(CC) $(CFLAGS) -o $@ $^
	cp $@ build/$@

$(BUILD_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<


clean:
	rm -rf $(BUILD_DIR) wasmkernel
