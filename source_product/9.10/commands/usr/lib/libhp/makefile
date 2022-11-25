# $Source: /misc/source_product/9.10/commands.rcs/usr/lib/libhp/makefile,v $
# $Revision: 64.3 $

# Common makefile for libhp
# Description file for hp: specials for Pascal, etc.
# Only supported on 300

LIB = libhp.a

OBJECTS = tryrec.pas.o tryrec.o

default:  $(LIB)

$(LIB):  $(OBJECTS)
	/bin/rm -f $(LIB)
	ar q $(LIB) $(OBJECTS)

clean:
	/bin/rm -f *.o

clobber:    clean
	/bin/rm -f $(LIB)
# copy /usr/lib/libhp to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libhp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! cp libhp.a ../../../../Filesets.sl/PROG-AUX/usr/lib/libhp.a
	! chmog 0444 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/libhp.a
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
