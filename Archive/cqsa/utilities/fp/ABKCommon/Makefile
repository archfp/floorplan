# include platform-specific definitions
include ../../../Makefile.defs

SRCS= \
abkassert.cxx \
abkcpunorm.cxx \
abkfunc.cxx \
abkio.cxx \
abkmessagebuf.cxx \
abkrand_templ.cxx \
abkroot.cxx \
abkstring.cxx \
abktimer.cxx \
crc32.cxx \
infolines.cxx \
mmapStreamBuf.cxx \
paramproc.cxx \
pathDelims.cxx \
platfDepend.cxx \
r1279.cxx \
r250.cxx \
seed.cxx \
verbosity.cxx \
Tausworthe.cxx \
MD5/md5.cxx \
SGI_STL_COMPAT/hashStatic.cxx

OBJS=$(SRCS:.cxx=.o)
DEPS=$(SRCS:.cxx=.d)

INCS=-I../
LIBS=
LDPATH=

all: libABKCommon

libABKCommon: $(OBJS)
	$(AR) libABKCommon.a $(OBJS)

clean:
	$(RM) libABKCommon.a $(OBJS) $(DEPS)

%.o: %.cxx
	$(CC) $< -o $@ $(CFLAGS) -c $(INCS) -MP -MMD

# include dependencies
-include $(DEPS)

