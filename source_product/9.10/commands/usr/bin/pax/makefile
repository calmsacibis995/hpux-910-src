# Common makefile for pax
#
# PAX - read and write POSIX conformant tar and cpio archives
#
# AUTHOR
#
#	Mark H. Colburn, Open Systems Architects, Inc. (mark@minnetech.mn.org)
#
# modified by: Michael Sieber, michas@hpbbn, Wed May 09 17:41:54 MESZ 1990
#

CFLAGS = -DDBUG_OFF

GENCAT= /usr/bin/gencat
FINDMSG= /usr/bin/findmsg -a

HEADERS= config.h dbug.h func.h limits.h port.h pax.h
SOURCE= hpux_rel.c pax.c append.c buffer.c cdf.c cpio.c create.c dbug.c\
	extract.c fileio.c link.c list.c mem.c msdos.c namelist.c names.c\
	pass.c pathname.c port.c regexp.c replace.c tar.c ttyio.c\
	warn.c wildmat.c 
OBJECT= hpux_rel.o pax.o append.o buffer.o cdf.o cpio.o create.o dbug.o\
	extract.o fileio.o link.o list.o mem.o msdos.o namelist.o names.o\
	pass.o pathname.o port.o regexp.o replace.o tar.o ttyio.o\
	warn.o wildmat.o 
MSGS=   pax.msg append.msg buffer.msg cpio.msg create.msg dbug.msg extract.msg\
	fileio.msg link.msg list.msg mem.msg msdos.msg namelist.msg names.msg\
	pass.msg pathname.msg port.msg regexp.msg replace.msg tar.msg ttyio.msg\
	warn.msg wildmat.msg 

PROGS= pax

default: pax pax.cat

metrics: pax.M

pax: $(OBJECT)
	$(CC) $(LDFLAGS) $(CFLAGS) -o pax $(OBJECT) 

pax.M: $(SOURCE)
	$(METRICS) $(CFLAGS) -o pax.M $(SOURCE)

clean:
	rm -f $(OBJECT) $(MSGS)

clobber: clean
	rm -f $(PROGS) a.out *.BAK *.bak pax.cat *.M

pax.cat: $(MSGS)
	$(GENCAT) $@ $(MSGS)

# We define two rules for creating .msg and .cat files.  With these
# rules, make knows how to automatically create .msg files from .c
# files and .cat files from .msg files.  This allows us to merely
# specify that the .cat depends on the .msg; make will create the
# .msg file for us if necessary.

.c.msg:
	$(FINDMSG) $*.c > $*.msg

.msg.cat:
	$(GENCAT) $*.cat $*.msg

.SUFFIXES:	.cat .msg

lint:
	lint $(LINTFLAGS) $(SOURCE)

$(OBJECT): $(HEADERS)

# copy /usr/bin/pax to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/bin/pax
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	pax
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/bin
	! cp pax ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin/pax
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
# copy /usr/bin/pax to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/bin/pax
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/bin
	! cp pax ../../../../Filesets.sl/UX-CORE/usr/bin/pax
	! chmog 0555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin/pax
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
