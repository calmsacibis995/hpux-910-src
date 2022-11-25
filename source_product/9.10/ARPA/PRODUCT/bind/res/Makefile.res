#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.15 (Berkeley) 11/21/87
#
SRCS=	herror.c res_comp.c res_debug.c res_init.c res_mkqry.c res_query.c \
	res_send.c named/gethost.c strcasecmp.c

OBJS=	herror.o res_comp.o res_debug.o res_init.o res_mkqry.o res_query.o \
	res_send.o gethost.o strcasecmp.o

BFASRCS=herror.b res_comp.b res_debug.b res_init.b res_mkqry.b \
	res_query.b res_send.b named/gethost.b strcasecmp.b

BFAOBJS=_herror.o _res_comp.o _res_debug.o _res_init.o _res_mkqry.o \
	_res_query.o _res_send.o _gethost.o _strcasecmp.o

DEFS=	-DDEBUG 
INCLUDE=../include
CFLAGS=	-O ${DEFS}
BCFLAGS	= -DBFA ${CFLAGS}
TAGSFILE=tags

BFACC=/usr/local/bin/bfacc1

.SUFFIXES:.b

libresolv.a: ${OBJS}
	ar cru libresolv.a ${OBJS}

libresolvg.a: ${OBJS}
	ar cru libresolvg.a ${OBJS}

.c.b:
	@-rm -f /users/pma/src/bind/bfa/$*.B $@ _$< _$*.o
	$(BFACC) ${BCFLAGS} -I${INCLUDE} -c -o $*.b $? -B"-d/tmp/bind.bfa/$*.B"

bfa: libresolvb.a

libresolvb.a: ${BFASRCS}
	ar r $@ $?

install: libresolv.a
	cp libresolv.a /usr/lib
#	ranlib /usr/lib/libresolv.a

.c.o:
	${CC} -I${INCLUDE} ${CFLAGS} -c $*.c

gethost.o: named/gethost.c
	${CC} -I${INCLUDE} -c ${CFLAGS} named/gethost.c

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f *.o *.b errs a.out core libresolv.a libresolvb.a tags

