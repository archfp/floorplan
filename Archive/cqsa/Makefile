# include platform-specific definitions
include Makefile.defs

# build all libraries and whatever binaries are needed at the moment
all: libs bins

libs: utilityLibs libmcs
bins: greedy

greedy: utilityLibs libmcs
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering greedy_ra/"
	cd greedy_ra/ && $(MAKE)

mcs: utilityLibs libmcs
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering mcs/"
	cd mcs/ && $(MAKE) mcs

libmcs:
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering mcs/"
	cd mcs/ && $(MAKE)

utilityLibs:
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering utilities/"
	cd utilities/ && $(MAKE)

clean:
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering greedy_ra/"
	cd greedy_ra/ && $(MAKE) clean
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering mcs/"
	cd mcs/ && $(MAKE) clean
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo "Entering utilities/"
	cd utilities/ && $(MAKE) clean
