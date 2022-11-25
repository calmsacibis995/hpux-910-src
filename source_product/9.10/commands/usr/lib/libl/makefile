# $Source: /misc/source_product/9.10/commands.rcs/usr/lib/libl/makefile,v $
# $Revision: 64.2.1.5 $

# Common makefile for libl
# The 300 makefile was also copying ncform and nrform to /usr/lib/lex, this
# will be done by the build process.
# Also the 800 makefile was linking ncform nrform to /usr/lib/lex, this will
# also be done by the build process.

AR	= /bin/ar
OBJECTS	= hpux_rel.o allprint.o main.o reject.o yyless.o yywrap.o
LIB	= libl.a

default:	$(LIB)

$(LIB): $(OBJECTS)
	/bin/rm -f $(LIB)
	$(AR) q $(LIB) `lorder $(OBJECTS) | tsort`

clean:
	/bin/rm -f *.o

clobber:	clean
	/bin/rm -f $(LIB) 
# copy /usr/lib/libl to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libl
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib/lex
	! cp ncform ../../../../Filesets.sl/BUILDENV/usr/lib/lex/ncform
	! chmog 0444 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/lex/ncform
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/lex
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libl to the . relative fileset C-AUX
# change mode, owner, and group on /usr/lib/libl
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-AUX/usr/lib/lex
	! cp ncform ../../../../Filesets.sl/C-AUX/usr/lib/lex/ncform
	! chmog 0444 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/lex/ncform
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/lex
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libl to the . relative fileset C-AUX
# change mode, owner, and group on /usr/lib/libl
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-AUX/usr/lib/lex
	! cp nrform ../../../../Filesets.sl/C-AUX/usr/lib/lex/nrform
	! chmog 0444 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/lex/nrform
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/lex
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libl to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libl
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! cp libl.a ../../../../Filesets.sl/BUILDENV/usr/lib/libl.a
	! chmog 0444 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/libl.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libl to the . relative fileset C-AUX
# change mode, owner, and group on /usr/lib/libl
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-AUX/usr/lib
	! cp libl.a ../../../../Filesets.sl/C-AUX/usr/lib/libl.a
	! chmog 0444 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/libl.a
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
