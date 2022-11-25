# $Header: makefile,v 66.1 89/11/14 16:26:26 smp Exp $

# Common makefile for rdiff command of rcs utility

INC	 = ../include
PFLAGS   = -I$(INC)

OBJS     = diff.o \
	   diffreg.o \
	   ../hpux_rel.o

default: rdiff

rdiff:   $(OBJS)
	 $(CC) $(LDFLAGS) $(OBJS) -o rdiff

clean:	 
	 /bin/rm -f $(OBJS)

clobber: clean
	 /bin/rm -f rdiff

$(OBJS): diff.h $(INC)/system.h $(INC)/bin.h

# Redefine the .c to .o rule as to include PFLAGS
.c.o:
	$(CC) $(CFLAGS) $(PFLAGS) -c $<
