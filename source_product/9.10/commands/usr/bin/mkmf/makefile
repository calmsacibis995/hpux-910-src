# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/mkmf/makefile,v $
# $Revision: 70.1.1.14 $

# Common makefile for mkmf utility
#
# The old make files used to also cd to mf and do some installing, this
# will now be done by the build process.

SUBDIR	      = mkmf
# Added OTHER macro to allow passing of a define necessary in order to build
# a version of mkmf for the HP series 1200.  When the series 1200 C and C++
# cpps define a machine name for the series 1200, can add SEQ68K as a define
# in one of the header files for this product.
OTHER=

default:		
		for i in $(SUBDIR);		\
		do (				\
			cd $$i;			\
			$(MAKE) CFLAGS="$(CFLAGS) $(OTHER)"\
		); done

clean:	
		for i in $(SUBDIR);		\
		do (				\
			cd $$i;			\
			$(MAKE) CFLAGS="$(CFLAGS) $(OTHER)" clean\
		); done

clobber:	
		for i in $(SUBDIR);		\
		do (				\
			cd $$i;			\
			$(MAKE) CFLAGS="$(CFLAGS) $(OTHER)" clobber\
		); done

# copy /usr/bin/mkmf to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/mkmf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	mkmf
# copy /usr/bin/mkmf to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/mkmf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/bin
	! cp mkmf/mkmf ../../../../Filesets.sl/PROG-AUX/usr/bin/mkmf
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin/mkmf
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mkmf to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/mkmf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! cp mf/C++.L ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/C++.L
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/C++.L
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mkmf to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/mkmf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! cp mf/C++.l ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/C++.l
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/C++.l
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mkmf to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/mkmf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! cp mf/C++.p ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/C++.p
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/C++.p
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mkmf to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/mkmf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! cp mf/C.L ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/C.L
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/C.L
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mkmf to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/mkmf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! cp mf/C.l ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/C.l
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/C.l
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mkmf to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/mkmf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! cp mf/C.p ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/C.p
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/C.p
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mkmf to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/mkmf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! cp mf/C.t ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/C.t
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/C.t
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mkmf to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/mkmf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! cp mf/dir.p ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/dir.p
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/dir.p
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mkmf to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/mkmf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! cp mf/man.p ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/man.p
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf/man.p
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/mf
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
