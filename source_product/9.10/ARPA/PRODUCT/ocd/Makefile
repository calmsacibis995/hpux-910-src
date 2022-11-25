/*
#
#	$Header: Makefile,v 1.1.109.3 92/04/08 10:31:33 ash Exp $
#
*/

.SOURCE : $(SRCDIR:/^$/./)
.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

/* These are redefined so that the debug version of ocd (ocdebug) can
   be made without having to invoke bemake with -debug.
*/
SRCS=	$(SRCDIR:/^$/./)/ocd.c $(SRCDIR:/^$/./)/dfa_account.c
HDRS=	-I$(SRCDIR:/^$/./) -I$(SRCDIR:/^$/./)/../include -I/usr/include
OBJS=	ocd.o dfa_account.o
PRODUCTS= pcf

LDLIBRARIES=		../util/libutil.a

CCFLAGS=	-O $(NEWCCFLAGS) 
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g -DDEBUG  $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif

all:  build

ocd	::  $(SRCS)

ocdebug	:: $(SRCS)
	$(RM) $(RMFLAGS) $(OBJS)
	$(CC) -c $(CCFLAGS) -DDEBUG $(HDRS) $(SRCS)
	$(CC) -o $(TARGET) $(NEWCCFLAGS) $(LDFLAGS) $(OBJS) $(LDLIBRARIES)

build : $(TARGET:/^$/ocd/) $(TARGET:/^$/ocdebug/)
	echo "Building... $(TARGET)"

install:
	$(INSTALL) $(TARGET)
	$(INSTALL) $(PRODUCTS)

clean:
	$(RM) $(RMFLAGS) $(OBJS) core

clobber: clean
	$(RM) $(RMFLAGS) $(TARGET)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

