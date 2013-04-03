# include platform-specific definitions
include ../Makefile.defs

SRCS= \
src/Component.cpp \
src/ComponentLibrary.cpp \
src/EMFailureMechanism.cpp \
src/FailureMechanism.cpp \
src/ManufacturingDefect.cpp \
src/TaskNet.cpp \
src/ComponentNet.cpp \
src/System.cpp \
src/Task.cpp \
src/TCFailureMechanism.cpp \
src/TDDBFailureMechanism.cpp \
src/TaskMapping.cpp \
src/config.cpp \
src/power.cpp

CSRCS= \
src/orionWrapper.c

OBJS=$(SRCS:.cpp=.o)
COBJS=$(CSRCS:.c=.o)
DEPS=$(SRCS:.cpp=.d)
CDEPS=$(CSRCS:.c=.d)

INCS+=-Iinclude -I../utilities -I../utilities/blockFiller -I../utilities/fp -I../utilities/fp/ParquetFP -I../utilities/orion
LIBS+=-lmcs -lhotSpot -lblockFiller -lParquetFP -lABKCommon -lpower
LDPATH+=-L. -L../utilities/hotSpot -L../utilities/blockFiller -L../utilities/fp/ParquetFP -L../utilities/fp/ABKCommon -L../utilities/orion

all: libmcs mcs

mcs: src/mcs.o libmcs
	$(CC) $< -o $@ $(CFLAGS) $(INCS) $(LIBS) $(LDPATH)

libmcs: $(OBJS) $(COBJS)
	$(AR) libmcs.a $(OBJS) $(COBJS)

clean:
	$(RM) libmcs.a mcs src/mcs.o $(OBJS) $(COBJS) src/mcs.d $(DEPS) $(CDEPS)

%.o: %.cpp
	$(CC) $< -o $@ $(CFLAGS) -c $(INCS) -MP -MMD

%.o: %.c
	gcc $< -o $@ $(CFLAGS) -c $(INCS) -MP -MMD -DPARM_TECH_POINT=10 -DVdd=1 -DPARM_Freq=1e8

# include dependencies
-include $(DEPS)
-include $(CDEPS)
