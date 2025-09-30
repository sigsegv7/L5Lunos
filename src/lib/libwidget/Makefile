CFILES = $(shell find src/ -name "*.c")
OBJECTS = $(CFILES:.c=.o)
CFLAGS = -pedantic -Iinclude/ -I../libc/include/ -L../libc/ -lc
CC = gcc
LIB_OUT = libwidget.a

.PHONY: all
all: $(OBJECTS)
	ar rcs $(LIB_OUT) $(OBJECTS)

%.o: %.c
	$(CC) -c -I$(CFLAGS) $< -o $@
