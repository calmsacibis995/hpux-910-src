#
# $Header: makefile,v 1.1.109.1 91/11/21 11:44:50 kcs Exp $
#
SHELL		= /bin/sh
INCLUDE=-I../../include
LIBRESOLV	= herror.o res_comp.o res_debug.o res_init.o res_mkqry.o \
		  res_query.o res_send.o gethost.o _res.o 

TMPCFLAGS	= ${CFLAGS} -DHP_NFS -DDEBUG -O 

all: 
	CFLAGS="${TMPCFLAGS}" $(MAKE) libresolv.a
	
libresolv.a: ${LIBRESOLV}
	ar cru $@ ${LIBRESOLV}

.c.o:
	${CC} ${INCLUDE} ${CFLAGS} -c $*.c

gethost.o: named/gethost.c
	${CC} ${INCLUDE} -c ${CFLAGS} named/gethost.c

clean:
	rm -f *.o *.b errs a.out core 

clobber: clean
	-rm -f libresolv.a libresolvg.a libresolvb.a
