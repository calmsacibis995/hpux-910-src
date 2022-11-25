#
#  $Source: /misc/source_product/9.10/commands.rcs/usr/bin/elm/makefile,v $
#  @(#) $Revision: 72.3.1.15 $
#
#  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
#  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
#
#  Acknowledgment is made to Dave Taylor for his creation of
#  the original version of this software.
#
#
#  Makefile for the entire ELM mail system
#


BIN	= bin
TOUCH	= /usr/bin/touch
MKDIR	= /usr/bin/mkdir

# NO_VM is defined because the code after virtually all the 	
#   vfork() calls is misbehaved in one way or another and it is not worth
#   the QA effort to assure that indiviual calls work.  Besides, in 8.0
#   all forks should be vforks :-)
DEFINE	= -DNO_VM	
LIBS	= -lcurses
RM	= /usr/bin/rm -f

UTILSRC	=			\
	utils/checkalias.c	\
	utils/elmalias.c	\
	utils/expand.c		\
	utils/listalias.c	\
	utils/mailfrom.c	\
	utils/newmail.c		\
	utils/readmail.c	\
	src/validname.c		\
	src/opt_utils.c		\
	src/getopt.c		\
	src/string2.c

ELMSRC	=			\
	src/addr_utils.c	\
	src/alias.c		\
	src/aliasdb.c		\
	src/aliaslib.c		\
	src/args.c		\
	src/bounceback.c	\
	src/builtin.c		\
	src/calendar.c		\
	src/checkname.c		\
	src/connect_to.c	\
	src/curses.c		\
	src/date.c		\
	src/delete.c		\
	src/del_alias.c		\
	src/domains.c		\
	src/edit.c		\
	src/editmsg.c		\
	src/elm.c		\
	src/encode.c		\
	src/errno.c		\
	src/expires.c		\
	src/file.c		\
	src/file_utils.c	\
	src/fileio.c		\
	src/forms.c		\
	src/getopt.c		\
	src/hdrconfg.c		\
	src/hdrconfg_b.c	\
	src/help.c		\
	src/initialize.c	\
	src/in_utils.c		\
	src/leavembox.c		\
	src/limit.c		\
	src/mailmsg1.c		\
	src/mailmsg2.c		\
	src/mailtime.c		\
	src/mkhdrs.c		\
	src/newmbox.c		\
	src/opt_utils.c		\
	src/options.c		\
	src/out_utils.c		\
	src/pattern.c		\
	src/pmalloc.c		\
	src/quit.c		\
	src/read_rc.c		\
	src/remail.c		\
	src/reply.c		\
	src/returnaddr.c	\
	src/save_opts.c		\
	src/savecopy.c		\
	src/screen.c		\
	src/showmsg.c		\
	src/showmsgcmd.c	\
	src/signals.c		\
	src/softkeys.c		\
	src/sort.c		\
	src/string2.c		\
	src/strings.c		\
	src/syscall.c		\
	src/utils.c		\
	src/validname.c		\
	src/version.mstr

MISCHDRS=			\
	hdrs/defs.h		\
	hdrs/save_opts.h	\
	hdrs/sysdefs.h		\
	hdrs/curses.h

ELMHDRS	=			\
	hdrs/headers.h 

MAINHDRS=			\
	hdrs/elm.h


default: all

all: ${BIN}/elm ${BIN}/utils 

metrics: ${BIN}/elm.M ${BIN}/utils.M

${BIN}:
	if [ ! -d ${BIN} ]; then ${MKDIR} ${BIN}; fi

elm: ${BIN}/elm

${BIN}/elm: ${ELMSRC} ${MISCHDRS} ${ELMHDRS} ${MAINHDRS}
	[ -d ${BIN} ] || mkdir ${BIN};	\
	cd src;				\
	$(MAKE) 'DEFINE=${DEFINE}' 'LIB2=${LIB2}' 'CFLAGS=${CFLAGS}' default

${BIN}/elm.M: ${ELMSRC} ${MISCHDRS} ${ELMHDRS} ${MAINHDRS} ${BIN}
	[ -d ${BIN} ] || mkdir ${BIN};	\
	cd src;				\
	$(MAKE) 'DEFINE=${DEFINE}' 'LIB2=${LIB2}' metrics
	
utils: ${BIN}/utils

${BIN}/utils: ${UTILSRC} ${MISCHDRS}
	[ -d ${BIN} ] || mkdir ${BIN};	\
	cd utils;			\
	$(MAKE) 'DEFINE=${DEFINE}' 'LIBS=${LIB2}' 'CFLAGS=${CFLAGS}' default

${BIN}/utils.M: ${UTILSRC} ${MISCHDRS}
	[ -d ${BIN} ] || mkdir ${BIN};	\
	cd utils;			\
	$(MAKE) 'DEFINE=${DEFINE}' 'LIBS=${LIB2}' metrics

src/encode.c:
	if [ ! -f src/encode.c ]; then ${TOUCH} src/encode.c; fi

clean:
	cd src; $(MAKE) clean
	cd utils; $(MAKE) clean

clobber:
	cd src; $(MAKE) clobber
	cd utils; $(MAKE) clobber
	rm -rf ${BIN}

# copy /usr/bin/elm to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/elm
# to 02555, bin, and mail
# change mode, owner, and group on all the parent
# directories.

install:	elm
# copy /usr/bin/elm to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/elm
# to 02555, bin, and mail
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/bin
	! cp bin/elm ../../../../Filesets.sl/MAILERS/usr/bin/elm
	! chmog 02555 bin mail ../../../../Filesets.sl/MAILERS/usr/bin/elm
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/elm to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/elm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/bin
	! cp bin/elmalias ../../../../Filesets.sl/MAILERS/usr/bin/elmalias
	! chmog 0555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin/elmalias
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/elm to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/elm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/bin
	! cp bin/mailfrom ../../../../Filesets.sl/MAILERS/usr/bin/mailfrom
	! chmog 0555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin/mailfrom
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/elm to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/elm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/bin
	! cp bin/newmail ../../../../Filesets.sl/MAILERS/usr/bin/newmail
	! chmog 0555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin/newmail
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/elm to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/elm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/bin
	! cp bin/readmail ../../../../Filesets.sl/MAILERS/usr/bin/readmail
	! chmog 0555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin/readmail
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/elm to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/elm
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/lib/elm
	! cp lib/elm-help.0 ../../../../Filesets.sl/MAILERS/usr/lib/elm/elm-help.0
	! chmog 0444 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/elm/elm-help.0
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/elm
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/elm to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/elm
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/lib/elm
	! cp lib/elm-help.1 ../../../../Filesets.sl/MAILERS/usr/lib/elm/elm-help.1
	! chmog 0444 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/elm/elm-help.1
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/elm
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/elm to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/elm
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/lib/elm
	! cp lib/elm-help.2 ../../../../Filesets.sl/MAILERS/usr/lib/elm/elm-help.2
	! chmog 0444 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/elm/elm-help.2
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/elm
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/elm to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/elm
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/lib/elm
	! cp lib/elmrc-info ../../../../Filesets.sl/MAILERS/usr/lib/elm/elmrc-info
	! chmog 0444 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/elm/elmrc-info
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/elm
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/elm to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/elm
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/lib/nls/C
	! cp src/elm.cat ../../../../Filesets.sl/MAILERS/usr/lib/nls/C/elm.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/nls/C/elm.cat
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
