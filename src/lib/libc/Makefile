OBJ = $(shell find build/ -name "*.o")
LIBC_OUT = libc.a

.PHONY: all
all: build sys target
	ar rcs $(LIBC_OUT) $(OBJ)

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
	rm -rf build/
