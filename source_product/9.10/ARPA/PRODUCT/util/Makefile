/*
 *
 *  $Header: Makefile,v 1.5.109.2 92/02/12 15:04:09 thchen Exp $
 *  New make file for libutil.a
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)
.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

SRCS= getpty.c utimes.c signal.c account.c logwtmp.c getwd.c secure.c
OBJS= getpty.o utimes.o signal.o account.o logwtmp.o getwd.o secure.o

CCFLAGS=	-O $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif /* Debug */

#ifdef BFA
BFADB=		/bfa/ARPA/databases/libutil.B
CC=		bfacc
CCFLAGS=	-B-d$(BFADB) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif /* BFA */

libutil.a :: $(OBJS)

all : build

build : $(TARGET:/^$/libutil.a/)

debug : build

alpha : build

bfa : build

install:
	$(INSTALL) $(TARGET:/^$/libutil.a/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/libutil.a/)

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/libutil.a/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p -I$(SRCDIR:/^$/./) -I$(SRCDIR:/^$/./)/../include $(SRCS)
