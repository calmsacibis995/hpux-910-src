#
# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/audsysusr/makefile,v $
# @(#) $Revision: 64.2.1.3 $
# Common makefile for building audsys and audusr commands
#

OBJS   = audsys.o audusr.o
SRCS   = audusr.c audsys.c
HDRS   = aud_def.h aud_msg.h aud_err.h nl_aud_err.h nl_aud_msg.h

default: all

all: audsys audusr audsysusr.cat

audsys:	audsys.o
	$(CC) $(CFLAGS) -o $@ audsys.o

audusr:	audusr.o
	$(CC) $(CFLAGS) -o $@ audusr.o $(LIBS)

audsysusr.cat: aud_err.msg aud_msg.msg
	gencat audsysusr.cat aud_err.msg aud_msg.msg

clean:
	/bin/rm -f $(OBJS) aud_err.msg aud_err.str aud_msg.msg aud_msg.str

clobber: clean
	/bin/rm -f audsys audusr audsysusr.cat \
	    nl_aud_err.h nl_aud_msg.h

lint:
	lint -abchpux $(SRCS)

size:
	size audsys audusr

tags:
	ctags $(SRCS)

wc:
	wc $(HDRS) $(SRCS)

audsys.o: $(HDRS)
audusr.o: $(HDRS)

nl_aud_err.h: aud_err.h
	-findstr aud_err.h >aud_err.str
	insertmsg aud_err.str >aud_err.msg

nl_aud_msg.h: aud_msg.h
	-findstr aud_msg.h >aud_msg.str
	insertmsg -n101 aud_msg.str >aud_msg.msg
# copy /usr/bin/audsysusr to the . relative fileset AUDIT
# change mode, owner, and group on /usr/bin/audsysusr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/AUDIT/usr/bin
	! cp audsys ../../../../Filesets.sl/AUDIT/usr/bin/audsys
	! chmog 0555 bin bin ../../../../Filesets.sl/AUDIT/usr/bin/audsys
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/audsysusr to the . relative fileset AUDIT
# change mode, owner, and group on /usr/bin/audsysusr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/AUDIT/usr/bin
	! cp audusr ../../../../Filesets.sl/AUDIT/usr/bin/audusr
	! chmog 0555 bin bin ../../../../Filesets.sl/AUDIT/usr/bin/audusr
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/audsysusr to the . relative fileset AUDIT
# change mode, owner, and group on /usr/bin/audsysusr
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/AUDIT/usr/lib/nls/C
	! cp audsysusr.cat ../../../../Filesets.sl/AUDIT/usr/lib/nls/C/audsysusr.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/AUDIT/usr/lib/nls/C/audsysusr.cat
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/
	! chmog 555 bin bin ../../../../Filesets.sl/
