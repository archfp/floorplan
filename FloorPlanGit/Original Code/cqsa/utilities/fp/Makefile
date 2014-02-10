# include platform-specific definitions
include ../../Makefile.defs

all: libs

libs: libABKCommon libParquetFP

libABKCommon:
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering ABKommon/"
	cd ABKCommon/ && $(MAKE)

libParquetFP:
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering ParquetFP/"
	cd ParquetFP/ && $(MAKE)

clean:
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering ABKommon/"
	cd ABKCommon/ && $(MAKE) clean
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering ParquetFP/"
	cd ParquetFP/ && $(MAKE) clean
