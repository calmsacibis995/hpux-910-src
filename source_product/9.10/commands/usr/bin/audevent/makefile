#
# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/audevent/makefile,v $
# @(#) $Revision: 64.1.1.5 $
#
# Common makefile for audevent
#

OBJS	=	_perror.o\
		basename.o\
		error.o\
		ev_index.o\
		get_opts.o\
		hpux_rel.o\
		main.o\
		mark_ev.o\
		mark_sc.o\
		sc_index.o\
		set_audit.o\
		show_audit.o\
		show_stat.o\
		usage.o

SRCS	=	$(OBJS:.o=.c)

MSGSRC	=	$(OBJS:.o=.msg)

DEFS	=	define.h\
		extern.h\
		global.h

FINDMSG	=	/usr/bin/findmsg
GENCAT	=	/usr/bin/gencat


default: all

all: audevent audevent.cat

audevent: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

audevent.cat: $(MSGSRC)
	$(GENCAT) $@ $(MSGSRC)

clean:
	rm -f core tags *.ln $(OBJS) $(MSGSRC)

clobber: clean
	/bin/rm -f audevent audevent.cat

lint:
	lint -abchpux $(SRCS)

tags:
	ctags $(SRCS)

wc:
	wc $(DEFS) $(SRCS)

ev_index.o: define.h
ev_index.o: extern.h

get_opts.o: define.h
get_opts.o: extern.h

main.o: define.h
main.o: extern.h
main.o: global.h

mark_ev.o: define.h
mark_ev.o: extern.h

mark_sc.o: define.h
mark_sc.o: extern.h

sc_index.o: define.h
sc_index.o: extern.h

set_audit.o: define.h
set_audit.o: extern.h

show_audit.o: define.h
show_audit.o: extern.h

.c.msg:
	$(FINDMSG) $*.c > $*.msg

.msg.cat:
	$(GENCAT) $*.cat $*.msg

.SUFFIXES: .cat .msg

# copy /usr/bin/audevent to the . relative fileset AUDIT
# change mode, owner, and group on /usr/bin/audevent
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	audevent
	mkdir -p ../../../../Filesets.sl/AUDIT/usr/bin
	! cp audevent ../../../../Filesets.sl/AUDIT/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/AUDIT/usr/bin/audevent
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/
# copy /usr/bin/audevent to the . relative fileset AUDIT
# change mode, owner, and group on /usr/bin/audevent
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/AUDIT/usr/bin
	! cp audevent ../../../../Filesets.sl/AUDIT/usr/bin/audevent
	! chmog 0555 bin bin ../../../../Filesets.sl/AUDIT/usr/bin/audevent
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/audevent to the . relative fileset AUDIT
# change mode, owner, and group on /usr/bin/audevent
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/AUDIT/usr/lib/nls/C
	! cp audevent.cat ../../../../Filesets.sl/AUDIT/usr/lib/nls/C/audevent.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/AUDIT/usr/lib/nls/C/audevent.cat
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/AUDIT/
	! chmog 555 bin bin ../../../../Filesets.sl/
