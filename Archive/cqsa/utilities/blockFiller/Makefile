# include platform-specific definitions
include ../../Makefile.defs

SRCS=blockFiller.cpp
OBJS=$(SRCS:.cpp=.o)
DEPS=$(SRCS:.cpp=.d)

INCS+=-I. -I../../mcs/include
LIBS+=
LDPATH+=

all: libblockfiller

libblockfiller: $(OBJS)
	$(AR) libblockFiller.a $(OBJS)

clean:
	$(RM) libblockFiller.a $(OBJS) $(DEPS)

%.o: %.cpp
	$(CC) $< -o $@ $(CFLAGS) -c $(INCS) -MP -MMD

# include dependencies
-include $(DEPS)
