/*
** $Header: Makefile,v 1.37.109.4 93/11/11 17:42:46 randyc Exp $
*/

.SOURCE :   $(SRCDIR:/^$/./) 

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../../../include /usr/include

#ifdef BFA
CCFLAGS		= -DBFA -DBSD=43
CC		= bfacc
LDLIBRARIES	= ../../res/libresolv.a
#else
CCFLAGS		+= -DBSD=43 $(NEWCCFLAGS)
LDFLAGS		= -s $(NEWLDFLAGS)
#endif


NSLOOKUPSRC	= commands.c debug.c getinfo.c hosttable.c \
		  list.c main.c send.c skip.c subr.c switch.c \
		  gethostent.c
ALPHA_NSLOOKUPSRC = commands.c debug.c getinfo.c hosttable.c switch.c \
		  list.c main.c send.c skip.c subr.c strcasecmp.c \
		  gethostent.c
NSLOOKUP	= commands.o debug.o getinfo.o hosttable.o \
		  list.o main.o send.o skip.o subr.o switch.o \
		  gethostent.o
ALPHA_NSLOOKUP	= commands.o debug.o getinfo.o hosttable.o \
		  list.o main.o send.o skip.o subr.o strcasecmp.o \
		  switch.o gethostent.o
HDRS		= bsd.h res.h

all: $(TARGET:/^$/nslookup/)

build: $(TARGET:/^$/nslookup/)

bfa: $(TARGET:/^$/nslookup/)

nslookup:: $(NSLOOKUPSRC) 
	/bin/sh $(SRCDIR:/^$/./)/newvers.sh
	$(CC) -o nslookup $(LDFLAGS) $(NSLOOKUP) version.c -ll $(LDLIBRARIES)

alpha:: $(ALPHA_NSLOOKUPSRC)
	$(CC) -o nslookup $(ALPHA_NSLOOKUP) version.c -ll ../../res/libresolv.a -lBSD

commands.c: $(SRCDIR:/^$/./)/commands.l
	$(LEX) $(LEXFLAGS) $(>)
	$(MV) lex.yy.c $(>:BS=.c)

ccount:
	rm -f commands.c
	ccount -p -I../../include *.h *.l *.c

clean: 
	$(RM) $(RMFLAGS) *.o *.b core commands.c bfadbase version.c

clobber: clean
	$(RM) $(RMFLAGS) nslookup $(MAKEFILE).mo $(MAKEFILE).ms

install: 
	$(INSTALL) nslookup

rinstall: install
	$(RINSTALL) nslookup


