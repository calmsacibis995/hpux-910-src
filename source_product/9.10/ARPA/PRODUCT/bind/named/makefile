#
# $Header: makefile,v 1.1.109.1 91/11/21 11:41:18 kcs Exp $
#
INCLUDE=-I. -I../../include
TMPCFLAGS 	= ${CFLAGS} -O -DDEBUG -DSTATS -DSYSV 
LDFLAGS		= -s

NAMED		= db_dump.o db_load.o db_lookup.o db_reload.o db_save.o \
		  db_update.o ns_forw.o ns_init.o ns_main.o ns_maint.o \
		  ns_req.o ns_resp.o ns_sort.o ns_stats.o db_glue.o \
		  strcasecmp.o sigvec.o 
XFER            = named-xfer.o db_glue.o strcasecmp.o sigvec.o 
HDRS		= db.h ns.h

.c.o:
	${CC} -c ${CFLAGS} ${INCLUDE} $*.c

all:
	CFLAGS="${TMPCFLAGS}" $(MAKE) named named-xfer

#
# This assumes the resolver library
# has been made with DEBUG compiled in.
#
named:	${NAMED} 
	sh newvers.sh
	${CC} -o $@ ${LDFLAGS} version.c ${NAMED} ../res/libresolv.a 

named-xfer: ${XFER}
	${CC} -o named-xfer ${LDFLAGS} version.c ${XFER}  ../res/libresolv.a

sigvec.o:
	ar xv /usr/lib/libBSD.a sigvec.o

clean: .FORCE
	rm -f *.o *.b core 

clobber: clean
	-rm -f named named.bfa named.cdb

.FORCE:
	@rm -f .FORCE

