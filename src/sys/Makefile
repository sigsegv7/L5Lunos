include conf/sys.mk

TARGETDIR = arch/$(TARGET)
CC =
LD =
AS =

.PHONY: all
all: md

.PHONY: md
md:
	cd $(TARGETDIR); make CC=$(CC) LD=$(LD) AS=$(AS)

.PHONY: clean
clean:
	cd $(TARGETDIR); make clean
