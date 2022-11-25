
#	@(#)lib.mk	2.11 DKHOST 87/03/20
#
#	COMMKIT(TM) Software - Datakit(R) VCS Interface Release 2.0 V1
#			Copyright 1984 AT&T
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#     The copyright notice above does not evidence any actual
#          or intended publication of such source code.
#

# installation directories and info
#ROOT	= ../..
ROOT = ..
DESTROOT = $(ROOT)
#USR	= /usr
USR	= 

LIBDIR	= $(DESTROOT)$(USR)/lib
LIBMODE	= 0555
OWNER	= bin
GROUP	= bin
CP	= cp
MV	= mv	

# makefile conventions
E	= @
ECHO	= @-echo
FRC	=
MKOPTS	= -e

INCLUDIR	= $(ROOT)$(USR)/include
INS		= sh filecopy $(LIBDIR)
CFLAGS	= -I$(INCLUDIR) -O 

OBJS = 	dkcanon.o\
	dkdial.o\
	dkxenviron.o\
	dkerr.o\
	dkexec.o\
	dkmgr.o\
	dkminor.o\
	dknamer.o\
	dksplice.o\
	dksplwait.o\
	dkxwrite.o\
	dkxstdio.o\
	dkbreak.o \
	maphost.o\
	dtnamer.o\
	dxnamer.o\
	dkeof.o\
	pupucommon.o\
	pull.o\
	push.o

default all: libdk.a 

install: 
	$(INS)  libdk.a
	cpset libdk.a ../../ 666 uucp other

libdk.a: $(OBJS) 
	ar crv libdk.a `lorder $(OBJS) | tsort`

clean:
	-rm -f *.o  libdk.a a.out core

clobber:
	-rm $(LIBDIR)/libdk.a

#  copy:
#	$(CPSET) libdk.a $(LIBDIR)/libdk.a $(LIBMODE) $(OWNER) $(GROUP)


# Include file dependencies added by Getincl, and modified
# Thu Apr 24 15:49:38 EST 1986, 4/29/86

dkbreak.o:	$(INCLUDIR)/dk.h
dkdial.o:	$(INCLUDIR)/dk.h
dkdial.o:	$(INCLUDIR)/sysexits.h
dkxenviron.o:	$(INCLUDIR)/dk.h
dkxenviron.o:	$(INCLUDIR)/sysexits.h
dkeof.o:	$(INCLUDIR)/dk.h
dkerr.o:	$(INCLUDIR)/sysexits.h
dkexec.o:	$(INCLUDIR)/dk.h
dkmgr.o:	$(INCLUDIR)/dk.h
dksplice.o:	$(INCLUDIR)/dk.h
dksplwait.o:	$(INCLUDIR)/dk.h
dkxstdio.o:	$(INCLUDIR)/sys/remfio.h
dkxstdio.o:	$(INCLUDIR)/dk.h
dkxstdio.o:	$(INCLUDIR)/sysexits.h
dkxwrite.o:	$(INCLUDIR)/dk.h
dkxwrite.o:	$(INCLUDIR)/sys/remfio.h
dkxwrite.o:	$(INCLUDIR)/sysexits.h
maphost.o:	$(INCLUDIR)/dk.h
