# include platform-specific definitions
include ../../Makefile.defs

SRCS=src/cfg2blocks.cpp
OBJS=$(SRCS:.cpp=.o)
DEPS=$(SRCS:.cpp=.d)

INCS+=-Iinclude -I../../mcs/include -I../blockFiller
LIBS+=-lmcs -lhotSpot -lblockFiller -lParquetFP -lABKCommon -lpower
LDPATH+=-L../../mcs -L../hotSpot -L../blockFiller -L../fp/ParquetFP -L../fp/ABKCommon -L../orion

all: cfg2blocks

cfg2blocks: $(OBJS)
	$(CC) $< -o $@ $(CFLAGS) $(INCS) $(LIBS) $(LDPATH)

clean:
	$(RM) cfg2blocks $(OBJS) $(DEPS)

%.o: %.cpp
	$(CC) $< -o $@ $(CFLAGS) -c $(INCS) -MP -MMD

# include dependencies
-include $(DEPS)
