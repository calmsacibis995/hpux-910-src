# $Header: makefile,v 64.2 89/01/19 15:46:50 benji Exp $
# @(#) $Revision: 64.2 $
#
# makefile -- makefile for subroutines used by accounting commands
#
LIB  = a.a
INC  = ..
OBJS = pnpsplit.o devtolin.o expand.o lintodev.o namtouid.o tmless.o \
       tmsecs.o uidtonam.o

default:	$(LIB)

$(LIB): $(OBJS)
	/bin/rm -f $(LIB)
	ar qv $(LIB) $(OBJS)
	@echo lib $(LIB) is now up to date

clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f $(LIB)

.c.o:
	$(CC) $(CFLAGS) -I$(INC) -c $<
