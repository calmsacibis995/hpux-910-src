# $Header: makefile,v 66.3 90/07/31 11:29:07 rsh Exp $

# Common makefile for rlog command of rcs utility

INC	 = ../include
PFLAGS 	 = -I$(INC)
LIBS     = ../lib/librcs.a -lBUILD

default: rlog

rlog:    rlog.o ../hpux_rel.o
	 $(CC) $(LDFLAGS) rlog.o ../hpux_rel.o $(LIBS) -o rlog

clean:		
	 /bin/rm -f rlog.o

clobber: clean
	 /bin/rm -f rlog

rlog.o:	 $(INC)/system.h $(INC)/time.h \
	 $(INC)/rcsbase.h

# Redefine the .c to .o rule as to include PFLAGS
.c.o:
	$(CC) $(CFLAGS) $(PFLAGS) -c $<
