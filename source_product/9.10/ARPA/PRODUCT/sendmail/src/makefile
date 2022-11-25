#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation,
# advertising materials, and other materials related to such
# distribution and use acknowledge that the software was developed
# by the University of California, Berkeley.  The name of the
# University may not be used to endorse or promote products derived
# from this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
# WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#	@(#)Makefile	5.11 (Berkeley) 1/3/89
#
#ifdef PATCH_STRING
# static char *patch_3997="@(#) PATCH_9.03: makefile $Revision: 1.22.109.5 $ 94/03/24 PHNE_3997";
#endif
SRCS=	alias.c arpadate.c clock.c codes.c collect.c conf.c convtime.c \
	daemon.c deliver.c domain.c envelope.c err.c headers.c macro.c \
	main.c mimefy.c parseaddr.c queue.c readcf.c recipient.c remote.c \
	savemail.c srvrsmtp.c stab.c stats.c sysexits.c trace.c usersmtp.c \
	util.c sem.c getpwentNS.c version.c
OBJS=	alias.o arpadate.o clock.o codes.o collect.o conf.o convtime.o \
	daemon.o deliver.o domain.o envelope.o err.o headers.o macro.o \
	main.o mimefy.o parseaddr.o queue.o readcf.o recipient.o remote.o \
	savemail.o srvrsmtp.o stab.o stats.o sysexits.o trace.o usersmtp.o \
	util.o sem.o getpwentNS.o version.o


CFLAGS=	-O -DDEBUG -DHP_NFS -I. -I../include -I../../include
LIBS=	-s ../../util/libutil.a

all : sendmail

sendmail: ${OBJS}
	${CC} ${COPTS} -o $@ ${OBJS} -lndbm ${LIBS}

clean:
	rm -f ${OBJS} core  a.out Version.o

clobber: clean
	rm -f ${MAN} tags .depend sendmail
