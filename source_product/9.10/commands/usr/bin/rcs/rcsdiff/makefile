# $Header: makefile,v 66.2 90/07/31 11:28:26 rsh Exp $

# Common makefile for rcsdiff command of rcs utility

INC	 = ../include
PFLAGS	 = -I$(INC)
LIBS	 = ../lib/librcs.a -lBUILD

default: rcsdiff

rcsdiff: rcsdiff.o ../hpux_rel.o
	 $(CC) $(LDFLAGS) rcsdiff.o ../hpux_rel.o $(LIBS) -o rcsdiff

clean:		
	 /bin/rm -f rcsdiff.o

clobber: clean
	 /bin/rm -f rcsdiff

rcsdiff.o: $(INC)/system.h $(INC)/bin.h \
	   $(INC)/rcsbase.h

# Redefine the .c to .o rule as to include PFLAGS
.c.o:
	$(CC) $(CFLAGS) $(PFLAGS) -c $<
