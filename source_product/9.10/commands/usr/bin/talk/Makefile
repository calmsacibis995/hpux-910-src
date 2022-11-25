# UNISRC_ID	@(#)37.4   87/02/25
# $Header: Makefile,v 72.1 92/11/20 14:38:45 ssa Exp $
#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.1 (Berkeley) 6/6/85
#
SHELL=/bin/sh
DESTDIR=
DOBJECT=talkd.o announce.o process.o table.o print.o ctl_cvt.o
TOBJECT=talk.o get_names.o display.o io.o ctl.o ctl_cvt.o init_disp.o \
	msgs.o get_addrs.o ctl_trans.o invite.o look_up.o
DSOURCE=talkd.c announce.c process.c table.c print.c ctl_cvt.c
TSOURCE=talk.c get_names.c display.c io.c ctl.c ctl_cvt.c init_disp.c \
	msgs.c get_addrs.c ctl_trans.c invite.c look_up.c
INCLUDE=talk.h ctl.h talk_ctl.h bsd.h
SOURCE= ${DSOURCE} ${TSOURCE}
CFLAGS= -O

LIBS= -lbsdipc

default:
	rm -f talk
	echo "#!/bin/sh"                                                >>talk
	echo "echo talk is not yet a supported command >&2"             >>talk
	echo "exit 1"                                                   >>talk
	chmod +x talk

all:	talk talkd

talkd: ${DOBJECT}
	$(CC) ${CFLAGS} -o talkd ${DOBJECT} ${LIBS}

talk:	${TOBJECT}
	$(CC) -o talk ${TOBJECT} -lcurses ${LIBS}
# don't need termlib for spectrum
#	$(CC) -o talk ${TOBJECT} -lcurses -ltermlib ${LIBS}

${DOBJECT}: ctl.h bsd.h
${TOBJECT}: talk.h ctl.h talk_ctl.h bsd.h

install: talk talkd
	$(INS) talk talkd README.talkd talk.1

shar:
	shar -cv Makefile ${SOURCE} ${INCLUDE} > talk.shar

lint:
	lint ${SOURCE} > lint

clean:
	rm -f *.o

clobber:	clean
	rm -f talk talkd lint

tags:	${SOURCE} ${INCLUDE}
	ctags ${SOURCE} ${INCLUDE}
