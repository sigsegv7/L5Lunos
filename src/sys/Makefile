include conf/sys.mk

TARGETDIR = arch/$(TARGET)
CC =
LD =
AS =

.PHONY: all
all: kern md

.PHONY: kern
kern:
	cd kern; make CC=$(CC) LD=$(LD) AS=$(AS) TARGETDIR=$(TARGETDIR)

.PHONY: md
md:
	cd $(TARGETDIR); make CC=$(CC) LD=$(LD) AS=$(AS)

.PHONY: clean
clean:
	cd $(TARGETDIR); make clean
	cd kern; make clean
