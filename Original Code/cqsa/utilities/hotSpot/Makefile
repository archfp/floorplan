# include platform-specific definitions
include ../../Makefile.defs

SRCS= \
flp.cpp \
flp_desc.cpp \
hotspot.cpp \
npe.cpp \
RCutil.cpp \
shape.cpp \
sim-template_block.cpp \
temperature.cpp \
temperature_block.cpp \
temperature_grid.cpp \
util.cpp

OBJS=$(SRCS:.cpp=.o)
DEPS=$(SRCS:.cpp=.d)

INCS+=
LIBS+=
LDPATH+=

all: libhotSpot

libhotSpot: $(OBJS)
	$(AR) libhotSpot.a $(OBJS)

clean:
	$(RM) libhotSpot.a $(OBJS) $(DEPS)

%.o: %.cpp
	$(CC) $< -o $@ $(CFLAGS) -c $(INCS) -MP -MMD

# include dependencies
-include $(DEPS)
