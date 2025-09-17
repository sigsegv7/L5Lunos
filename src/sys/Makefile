include conf/sys.mk

TARGETDIR = arch/$(TARGET)
CC =
LD =
AS =

.PHONY: all
all: os lib md

.PHONY: os
os:
	cd os; make CC=$(CC) LD=$(LD) AS=$(AS) TARGETDIR=$(TARGETDIR) CONF=$(CONF)

.PHONY: md
md:
	cd $(TARGETDIR); make CC=$(CC) LD=$(LD) AS=$(AS) CONF=$(CONF)

.PHONY: clean
clean:
	cd $(TARGETDIR); make clean
	cd os; make clean
