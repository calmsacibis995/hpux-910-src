# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/uupath/makefile,v $
# $Revision: 64.2.1.5 $

# Common makefile for uupath and mkuupath

LIBS = -ldbm

default: uupath mkuupath

uupath:
	$(CC) $(CFLAGS) uupath.c $(LDFLAGS) -o uupath $(LIBS)

mkuupath:
	$(CC) $(CFLAGS) mkuupath.c $(LDFLAGS) -o mkuupath $(LIBS)

clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f uupath mkuupath

# copy /usr/bin/uupath to the . relative fileset SYSCOM
# change mode, owner, and group on /usr/bin/uupath
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	uupath
	mkdir -p ../../../../Filesets.sl/SYSCOM/usr/bin
	! cp uupath ../../../../Filesets.sl/SYSCOM/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/SYSCOM/usr/bin/uupath
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/
# copy /usr/bin/uupath to the . relative fileset UUCP
# change mode, owner, and group on /usr/bin/uupath
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UUCP/usr/bin
	! cp mkuupath ../../../../Filesets.sl/UUCP/usr/bin/mkuupath
	! chmog 0555 bin bin ../../../../Filesets.sl/UUCP/usr/bin/mkuupath
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UUCP/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/uupath to the . relative fileset SYSCOM
# change mode, owner, and group on /usr/bin/uupath
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYSCOM/usr/bin
	! cp uupath ../../../../Filesets.sl/SYSCOM/usr/bin/uupath
	! chmog 0555 bin bin ../../../../Filesets.sl/SYSCOM/usr/bin/uupath
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/
	! chmog 555 bin bin ../../../../Filesets.sl/
