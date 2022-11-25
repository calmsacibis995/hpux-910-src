/*
 *
 *  $Header: Makefile,v 1.10.109.2 92/04/06 18:07:48 byrond Exp $
 *  New make file for remshd
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include


REMSHDSRC=	$(SRCDIR:/^$/./)/remshd.c
REMSHDOBJ=	remshd.o
REXECDSRC=	$(SRCDIR:/^$/./)/remshd.c
REXECDOBJ=	rexecd.o

SRCS=	$(SRCDIR:/^$/./)/
OBJS=	

LDLIBRARIES=	../util/libutil.a

CCFLAGS=	-O -DAUDIT -DHP_NFS $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g -DAUDIT -DHP_NFS $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif /* Debug */

#ifdef BFA
BFADB=		/bfa/ARPA/databases/$(TARGET:/^$/remshd-rexecd/).B
CC=		bfacc
CCFLAGS=	-B-d$(BFADB) -DAUDIT -DHP_NFS $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif /* BFA */

#ifdef TRUX
CCFLAGS+= -DTRUX -DSecureWare -DB1 -UAUDIT -DHP_NFS
#endif /* TRUX */

remshd : $(REMSHDOBJ) $(OBJS)
	$(CC) -o remshd $(REMSHDOBJ) $(OBJS) $(LDFLAGS) $(LDLIBRARIES)

remshd.o :
	$(CC) $(CCFLAGS) -DREMSHD -c $(>) -o $(REMSHDOBJ)

rexecd : $(REXECDOBJ) $(OBJS)
	$(CC) -o rexecd $(REXECDOBJ) $(OBJS) $(LDFLAGS) $(LDLIBRARIES)

rexecd.o : $(REMSHDSRC)
	$(CC) $(CCFLAGS) -DREXECD -c $(>) -o $(REXECDOBJ)

all : build

build : $(TARGET:/^$/remshd rexecd/)

debug : build

alpha : build

bfa : build

install:
	$(INSTALL) $(TARGET:/^$/remshd rexecd/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/remshd rexecd/)

clean :
	$(RM) $(RMFLAGS) $(TARGET:/^$/remshd.o rexecd/).o

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/remshd rexecd/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p -I$(SRCDIR:/^$/./) -I$(SRCDIR:/^$/./)/../include \
		-DAUDIT -DHP_NFS -DREMSHD *.[ch]
	ccount -p -I$(SRCDIR:/^$/./) -I$(SRCDIR:/^$/./)/../include \
		-DAUDIT -DHP_NFS -DREXECD *.[ch]
