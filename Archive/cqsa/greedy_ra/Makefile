# include platform-specific definitions
include ../Makefile.defs

SRCS=src/greedy.cpp
OBJS=$(SRCS:.cpp=.o)
DEPS=$(SRCS:.cpp=.d)

INCS+=-Iinclude -I../mcs/include -I../utilities/blockFiller
LIBS+=-lmcs -lhotSpot -lblockFiller -lParquetFP -lABKCommon -lpower
LDPATH+=-L../mcs -L../utilities/hotSpot -L../utilities/blockFiller -L../utilities/fp/ParquetFP -L../utilities/fp/ABKCommon -L../utilities/orion

all: greedy

greedy: $(OBJS)
	$(CC) $< -o $@ $(CFLAGS) $(INCS) $(LIBS) $(LDPATH)

clean:
	$(RM) greedy $(OBJS) $(DEPS)

%.o: %.cpp
	$(CC) $< -o $@ $(CFLAGS) -c $(INCS) -MP -MMD

# include dependencies
-include $(DEPS)
