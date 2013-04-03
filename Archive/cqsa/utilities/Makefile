# include platform-specific definitions
include ../Makefile.defs

# build all libraries and whatever binaries are needed at the moment
all: libs

libs: libblockFiller libhotSpot libParquetFP liborion

libblockFiller:
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering blockFiller/"
	cd blockFiller/ && $(MAKE)

libhotSpot:
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering hotSpot/"
	cd hotSpot/ && $(MAKE)

libParquetFP:
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering fp/"
	cd fp/ && $(MAKE)

liborion:
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering orion/"
	cd orion/ && $(MAKE) depend && $(MAKE)

clean:
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering blockFiller/"
	cd blockFiller/ && $(MAKE) clean
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering hotSpot/"
	cd hotSpot/ && $(MAKE) clean
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering fp/"
	cd fp/ && $(MAKE) clean
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering orion/"
	cd orion/ && $(MAKE) clean
