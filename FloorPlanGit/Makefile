PROG	 = ArchFP
OBJS     = Main.o Floorplan.o MathUtil.o
GPP	 = g++ -Wall

#Charles: Originally --> "-O0 -g"
CFLAGS	 = -O0 -g
#LDFLAGS = -lmcheck

$(PROG): $(OBJS) 
	$(GPP) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)


%.o: %.cc Floorplan.hh MathUtil.hh
	$(GPP) -c $< $(CFLAGS) -o $@

clean:
	rm -f $(PROG) *.o
