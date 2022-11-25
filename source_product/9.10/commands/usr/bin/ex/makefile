# $Header: makefile,v 66.3.1.14 94/07/29 09:03:37 root Exp $
# @(#) $Revision: 66.3.1.14 $
#
# makefile -- makefile for /usr/bin/ex and releated commands
#
FLAGS  = -DTABS=8 -DSINGLE -DUSG -DSTDIO -DVMUNIX -DVFORK
DEBUG  = -DTRACE -g $(FLAGS)
LIBS   = -lcurses
OBJS =	ex.o ex_addr.o ex_cmds.o ex_cmds2.o ex_cmdsub.o \
	ex_data.o ex_extern.o ex_get.o ex_io.o ex_put.o ex_re.o \
	ex_rldata.o ex_set.o ex_subr.o ex_temp.o ex_tty.o ex_unix.o \
	ex_v.o ex_vadj.o ex_vget.o ex_vmain.o ex_voper.o \
	ex_vops.o ex_vops2.o ex_vops3.o ex_vput.o ex_vwind.o \
	printf.o bcopy.o ex_re2.o ex_crypt.o
REL  =  hpux_rel.o
HDRS =	ex.h ex_argv.h ex_re.h ex_temp.h ex_tty.h ex_tune.h ex_vars.h \
	ex_vis.h
MSGSRC=	ex.c ex_addr.c ex_cmds.c ex_cmds2.c ex_cmdsub.c ex_get.c \
	ex_io.c ex_re.c ex_set.c ex_subr.c ex_temp.c ex_tty.c \
	ex_unix.c ex_v.c ex_vadj.c ex_vget.c ex_vmain.c ex_vops2.c \
	ex_vput.c printf.c
FINDMSG	= /usr/bin/findmsg
GENCAT	= /usr/bin/gencat
TOUCH   = /bin/touch

.c.o:
	$(CC) -c $(CFLAGS) $(FLAGS) $*.c

default: all

all:	ex exrecover expreserve ex.cat expreserve.cat exrecover.cat

ex: $(OBJS) $(REL)
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) $(REL) $(OBJS) $(LIBS) -o ex

exrecover: exrecover.o ex_extern.o $(REL)
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) -o $@ \
	    $(REL) exrecover.o ex_extern.o

expreserve: expreserve.o ex_extern.o $(REL)
	libs="";						\
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then	\
	    libs="$$libs -lscmd -lsec -lcurses";		\
	fi;							\
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) -o $@ \
	    $(REL) expreserve.o ex_extern.o $$libs

ex.cat:	ex.msg
	$(GENCAT) $@ ex.msg

ex.msg:	$(MSGSRC)
	rm -f $@
	for i in $(MSGSRC); do \
		$(FINDMSG) $$i >>$@;\
	done

exrecover.cat: exrecover.msg
	$(GENCAT) $@ exrecover.msg

exrecover.msg: exrecover.c
	$(FINDMSG) exrecover.c >$@

expreserve.cat: expreserve.msg
	$(GENCAT) $@ expreserve.msg

expreserve.msg: expreserve.c
	$(FINDMSG) expreserve.c >$@

#
# For some engineers, CRYPT-specific source is unavailable.  Touch the
# file so that the (non-CRYPT) make can work.
#
ex_crypt.o :
	$(TOUCH) ex_crypt.c
	$(CC) -c $(CFLAGS) $(FLAGS) ex_crypt.c

clean:
	rm -f *.o *.msg

clobber: clean
	rm -f ex exrecover expreserve *.cat

lint:
	$(LINT) $(CFLAGS) $(FLAGS) ex.c ex_?*.c
	$(LINT) $(CFLAGS) $(FLAGS) -u exrecover.c
	$(LINT) $(CFLAGS) $(FLAGS) expreserve.c

tags:	/tmp
	ctags -w ex.[hc] ex_*.[hc]

# copy /usr/bin/ex to the . relative fileset EDITORS
# change mode, owner, and group on /usr/bin/ex
# to 01555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	ex
	mkdir -p ../../../../Filesets.sl/EDITORS/usr/bin
	! cp ex ../../../../Filesets.sl/EDITORS/usr/bin
	! chmog 01555 bin bin ../../../../Filesets.sl/EDITORS/usr/bin/ex
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/
# copy /usr/bin/ex to the . relative fileset EDITORS
# change mode, owner, and group on /usr/bin/ex
# to 01555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/EDITORS/usr/bin
	! cp ex ../../../../Filesets.sl/EDITORS/usr/bin/ex
	! chmog 01555 bin bin ../../../../Filesets.sl/EDITORS/usr/bin/ex
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/ex to the . relative fileset EDITORS
# change mode, owner, and group on /usr/bin/ex
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/EDITORS/usr/lib
	! cp expreserve ../../../../Filesets.sl/EDITORS/usr/lib/expreserve
	! chmog 04555 root bin ../../../../Filesets.sl/EDITORS/usr/lib/expreserve
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/ex to the . relative fileset EDITORS
# change mode, owner, and group on /usr/bin/ex
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/EDITORS/usr/lib
	! cp exrecover ../../../../Filesets.sl/EDITORS/usr/lib/exrecover
	! chmog 04555 root bin ../../../../Filesets.sl/EDITORS/usr/lib/exrecover
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/ex to the . relative fileset EDITORS
# change mode, owner, and group on /usr/bin/ex
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/EDITORS/usr/lib/nls/C
	! cp ex.cat ../../../../Filesets.sl/EDITORS/usr/lib/nls/C/ex.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/EDITORS/usr/lib/nls/C/ex.cat
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/ex to the . relative fileset EDITORS
# change mode, owner, and group on /usr/bin/ex
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/EDITORS/usr/lib/nls/C
	! cp expreserve.cat ../../../../Filesets.sl/EDITORS/usr/lib/nls/C/expreserve.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/EDITORS/usr/lib/nls/C/expreserve.cat
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/ex to the . relative fileset EDITORS
# change mode, owner, and group on /usr/bin/ex
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/EDITORS/usr/lib/nls/C
	! cp exrecover.cat ../../../../Filesets.sl/EDITORS/usr/lib/nls/C/exrecover.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/EDITORS/usr/lib/nls/C/exrecover.cat
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/ex to the . relative fileset EDITORS
# change mode, owner, and group on /usr/bin/ex
# to 01555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/EDITORS/usr/bin
	! ln ../../../../Filesets.sl/EDITORS/usr/bin/ex ../../../../Filesets.sl/EDITORS/usr/bin/e
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/ex to the . relative fileset EDITORS
# change mode, owner, and group on /usr/bin/ex
# to 01555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/EDITORS/usr/bin
	! ln ../../../../Filesets.sl/EDITORS/usr/bin/ex ../../../../Filesets.sl/EDITORS/usr/bin/edit
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/ex to the . relative fileset EDITORS
# change mode, owner, and group on /usr/bin/ex
# to 01555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/EDITORS/usr/bin
	! ln ../../../../Filesets.sl/EDITORS/usr/bin/ex ../../../../Filesets.sl/EDITORS/usr/bin/vedit
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/ex to the . relative fileset EDITORS
# change mode, owner, and group on /usr/bin/ex
# to 01555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/EDITORS/usr/bin
	! ln ../../../../Filesets.sl/EDITORS/usr/bin/ex ../../../../Filesets.sl/EDITORS/usr/bin/vi
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/ex to the . relative fileset EDITORS
# change mode, owner, and group on /usr/bin/ex
# to 01555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/EDITORS/usr/bin
	! ln ../../../../Filesets.sl/EDITORS/usr/bin/ex ../../../../Filesets.sl/EDITORS/usr/bin/view
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/EDITORS/
	! chmog 555 bin bin ../../../../Filesets.sl/
