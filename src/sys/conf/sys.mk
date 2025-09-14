TARGET = amd64  # default
LDSCRIPT = ../md/$(TARGET)/conf/sys.ld   # default

# TODO: Make less rigid
override MD_CFLAGS = -mno-80387 -mno-mmx -mno-3dnow \
	-mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel

override MI_CFLAGS = -fexceptions --std=gnu11 -ffreestanding -fno-stack-protector -fno-pic \
	-Werror=implicit -Werror=implicit-function-declaration \
	-Werror=implicit-int -Werror=int-conversion \
    -Werror=missing-prototypes  -Isys/include/lib/ \
	-Werror=incompatible-pointer-types -Werror=int-to-pointer-cast \
	-Werror=return-type -mno-red-zone -mcmodel=kernel \
	-D_KERNEL -Wno-pointer-sign -MMD -nostdinc \
    -Wno-format-pedantic -Wno-attributes -D_KERNEL $(MD_CFLAGS)
