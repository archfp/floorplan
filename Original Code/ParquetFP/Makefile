# include platform-specific definitions
include ../../../Makefile.defs

SRCS= \
baseannealer.cxx \
basepacking.cxx \
btree.cxx \
btreeanneal.cxx \
btreecompact.cxx \
btreeslackeval.cxx \
mixedpacking.cxx \
mixedpackingfromdb.cxx \
netlist.cxx \
plcompact.cxx \
plsptobtree.cxx \
pltobtree.cxx \
\
AnalytSolve.cxx \
Annealer.cxx \
ClusterDB.cxx \
CommandLine.cxx \
DB.cxx \
FPcommon.cxx \
Net.cxx \
Nets.cxx \
Node.cxx \
Nodes.cxx \
Parquet.cxx \
PlToSP.cxx \
SPeval.cxx \
SeqPair.cxx \
SolveMulti.cxx

OBJS=$(SRCS:.cxx=.o)
DEPS=$(SRCS:.cxx=.d)

INCS=-I../
LIBS= -lParquetFP -lABKCommon
LDPATH=-L../ABKCommon/ -L.

all: libParquetFP ParquetFP

libParquetFP: $(OBJS)
	$(AR) libParquetFP.a $(OBJS)

ParquetFP: main.o libParquetFP
	$(CC) main.o -o $@ $(CFLAGS) $(INCS) $(LIBS) $(LDPATH)

clean:
	$(RM) libParquetFP.a main.o main.d ParquetFP $(OBJS) $(DEPS)

%.o: %.cxx
	$(CC) $< -o $@ $(CFLAGS) -c $(INCS) -MP -MMD

# include dependencies
-include $(DEPS)
