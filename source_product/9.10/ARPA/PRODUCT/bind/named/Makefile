/*
** $Header: Makefile,v 1.45.109.1 91/11/21 11:40:04 kcs Exp $
*/

.SOURCE :   $(SRCDIR:/^$/./) 

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../../include /usr/include

#ifdef BFA
CCFLAGS		= -DBFA -DDEBUG -DSTATS -DSYSV 
CC		= bfacc
LDLIBRARIES	= ../res/libresolv.a
#else
CCFLAGS		+= -O -DDEBUG -DSTATS -DSYSV $(NEWCCFLAGS)
#endif

#ifdef CDB
CCFLAGS		+= -g 
#endif

LDFLAGS		= -s $(NEWLDFLAGS)


NAMEDSRC	= db_dump.c db_load.c db_lookup.c db_reload.c db_save.c \
		  db_update.c ns_forw.c ns_init.c ns_main.c ns_maint.c \
		  ns_req.c ns_resp.c ns_sort.c ns_stats.c db_glue.c \
		  strcasecmp.c sigvec.o 
NAMED		= db_dump.o db_load.o db_lookup.o db_reload.o db_save.o \
		  db_update.o ns_forw.o ns_init.o ns_main.o ns_maint.o \
		  ns_req.o ns_resp.o ns_sort.o ns_stats.o db_glue.o \
		  strcasecmp.o sigvec.o 
ALPHA_NAMED	= db_dump.o db_load.o db_lookup.o db_reload.o db_save.o \
		  db_update.o ns_forw.o ns_init.o ns_main.o ns_maint.o \
		  ns_req.o ns_resp.o ns_sort.o ns_stats.o db_glue.o \
		  strcasecmp.o sigvec.o bzero.o bcopy.o bcmp.o \
		  index.o
XFERSRC         = named-xfer.c db_glue.c strcasecmp.c sigvec.o  
XFER            = named-xfer.o db_glue.o strcasecmp.o sigvec.o  
ALPHA_XFER      = named-xfer.o db_glue.o strcasecmp.o \
		  sigvec.o  mktemp.o bcopy.o bzero.o index.o bcmp.o
HDRS		= db.h ns.h

all: $(TARGET:/^$/named named-xfer/)

build: $(TARGET:/^$/named named-xfer/)

bfa: $(TARGET:/^$/named named-xfer/)

/*
** This assumes the resolver library
** has been made with DEBUG compiled in.
*/

named::	$(NAMEDSRC) 
	/bin/sh $(SRCDIR:/^$/./)/newvers.sh
	$(CC) -o named $(LDFLAGS) $(NAMED) version.c $(LDLIBRARIES)

named-xfer:: $(XFERSRC)
	$(CC) -o named-xfer $(LDFLAGS) $(XFER) version.c $(LDLIBRARIES)

/*
** Use include directory
*/

alpha: .FOREGROUND
	$(MAKE) CCFLAGS="$(CCFLAGS) -DHPALPHA" alpha_named alpha_xfer


/*
** Don't strip for alpha test
*/

alpha_named::	$(ALPHA_NAMED) 
	/bin/sh ./newvers.sh
	$(CC) -o named $(ALPHA_NAMED) version.c ../res/libresolv.a 

alpha_xfer:: $(ALPHA_XFER)
	$(CC) -o named-xfer $(ALPHA_XFER) version.c ../res/libresolv.a

named.cdb:: $(NAMEDSRC) 
	$(CC) -o named.cdb $(LDFLAGS) $(ALPHA_NAMED) version.c ../res/libresolv.a 

bzero.o:
	ar xv /usr/lib/libBSD.a bzero.o

bcopy.o:
	ar xv /usr/lib/libBSD.a bcopy.o

bcmp.o:
	ar xv /usr/lib/libBSD.a bcmp.o

index.o:
	ar xv /usr/lib/libBSD.a index.o

sigvec.o:
	ar xv /usr/lib/libBSD.a sigvec.o

ccount:
	ccount -p -I../include -DDEBUG -DSTATS -DSYSV *.h *.c

clean: 
	$(RM) $(RMFLAGS) *.o *.b core bfadbase

clobber: clean
	$(RM) $(RMFLAGS) named named.bfa named.cdb $(MAKEFILE).mo $(MAKEFILE).ms

install: 
	$(INSTALL) named named-xfer

rinstall: install
	$(RINSTALL) named named-xfer

