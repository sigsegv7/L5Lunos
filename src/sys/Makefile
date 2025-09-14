include conf/sys.mk

TARGETDIR = arch/$(TARGET)
CC =
LD =
AS =

.PHONY: all
all: os lib md

.PHONY: os
os:
	cd os; make CC=$(CC) LD=$(LD) AS=$(AS) TARGETDIR=$(TARGETDIR)

.PHONY: lib
lib:
	cd lib; make CC=$(CC) LD=$(LD) AS=$(AS) TARGETDIR=$(TARGETDIR)

.PHONY: md
md:
	cd $(TARGETDIR); make CC=$(CC) LD=$(LD) AS=$(AS)

.PHONY: clean
clean:
	cd $(TARGETDIR); make clean
	cd os; make clean
