#
# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/audisp/makefile,v $
# @(#) $Revision: 64.3.1.5 $
# Common makefile for building the audisp command
#

OBJS	= audisp.o hpux_rel.o translate.o
SRCS	= audisp.c translate.c
HDRS	= eventmap.h

default: all

all: audisp audisp.cat

audisp:	$(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

audisp.cat:
	findmsg audisp.c >audisp.msg
	gencat audisp.cat audisp.msg


clean:
	rm -f $(OBJS) audisp.msg

clobber: clean
	rm -f audisp audisp.cat

lint:
	lint -abchpux $(SRCS)

size:
	size audisp

tags:
	ctags $(SRCS)

wc:
	wc $(HDRS) $(SRCS)

audisp.o: $(HDRS)
	if hp9000s300; \
	then $(CC) $(CFLAGS) +O1 -c audisp.c  # optimizer bug for 7.0; \
	else $(CC) $(CFLAGS) -c audisp.c; \
	fi

translate.o: $(HDRS)
	if hp9000s300; \
	then $(CC) $(CFLAGS) +O1 -c translate.c  # optimizer bug for 7.0; \
	else $(CC) $(CFLAGS) -c translate.c; \
	fi

# copy /usr/bin/audisp to the . relative fileset AUDIT
# change mode, owner, and group on /usr/bin/audisp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	audisp
	mkdir -p ../../../../Filesets.sl/AUDIT/usr/bin
	! cp audisp ../../../../Filesets.sl/AUDIT/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/AUDIT/usr/bin/audisp
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/
# copy /usr/bin/audisp to the . relative fileset AUDIT
# change mode, owner, and group on /usr/bin/audisp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/AUDIT/usr/bin
	! cp audisp ../../../../Filesets.sl/AUDIT/usr/bin/audisp
	! chmog 0555 bin bin ../../../../Filesets.sl/AUDIT/usr/bin/audisp
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/audisp to the . relative fileset AUDIT
# change mode, owner, and group on /usr/bin/audisp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/AUDIT/usr/lib/nls/C
	! cp audisp.cat ../../../../Filesets.sl/AUDIT/usr/lib/nls/C/audisp.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/AUDIT/usr/lib/nls/C/audisp.cat
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/
	! chmog 555 bin bin ../../../../Filesets.sl/
