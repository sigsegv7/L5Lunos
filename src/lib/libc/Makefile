CFILES = $(shell find src/ -name "*.c")
CFILES += $(shell find/ musl-math -name "*.c")
OBJ = $(shell find build/ -name "*.o")
OBJ += $(CFILES:.c=.o)
LIBC_OUT = libc.a

.PHONY: all
all: build sys $(OBJ) target
	ar rcs $(LIBC_OUT) $(OBJ)

%.o: %.c
	$(CC) -ffreestanding -nostdlib -Iinclude/ -I$(TARGET)/include/ -c $< -o $@

.PHONY: target
target:
	cd $(TARGET); make CC=$(CC) LD=$(LD) AS=$(AS)

.PHONY: sys
sys:
	mkdir -p sys/
	rsync -av ../../sys/include/sys/* include/sys/

# Create build directory
build:
	mkdir -p $@

.PHONY: clean
clean:
	rm -rf build/ $(OBJ)
