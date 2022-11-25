# Makefile for portnls library routines and auxiliary
# message catalog files (/usr/lib/nls/$LANG/custdat.cat)
#

GENCAT=/usr/bin/gencat

NLSDIR=nls
LIB=libportnls.a
INC=include
PFLAGS	= -I$(INC)
VERSION=Uni_rel.o

HFILES=$(INC)/nlinfo.h

OBJECTS=hpux_rel.o	\
	almanac.o	\
	getnltable.o	\
	nlappend.o	\
	nlcollate.o	\
	nlcollsys.o	\
	nlconvclock.o	\
	nlconvcustda.o	\
	nlconvnum.o	\
	extract_nldt.o	\
	format_nldt.o	\
	nlcrtm.o	\
	nldate.o	\
	nlfindstr.o	\
	nlfmtcal.o	\
	nlfmtclock.o	\
	nlfmtcustdat.o	\
	nlfmtdate.o	\
	nlfmtlongcal.o	\
	nlfmtnum.o	\
	nlgetlang.o	\
	nlinfo.o	\
	nljudge.o	\
	nlkeycompare.o	\
	nlnumspec.o	\
	nlrepchar.o	\
	nlscanmove.o	\
	nlsubstr.o	\
	nlswitchbuf.o	\
	nltranslate.o	\
	nlutils.o	\
	utils.o


LANGS = american 	\
	arabic 		\
	arabic-w 	\
	c-french 	\
	chinese-s 	\
	chinese-t  	\
        danish 		\
	dutch 		\
	english 	\
	finnish 	\
	french 		\
	german 		\
	greek     	\
        hebrew 		\
	icelandic 	\
	italian 	\
	japanese 	\
	katakana 	\
	korean      	\
        n-computer 	\
	norwegian 	\
	portuguese 	\
	spanish 	\
	swedish 	\
	turkish

default: all

all:	expand catalogs $(LIB)

$(LIB): $(OBJECTS) 
	/bin/rm -f $(LIB)
	ar q $(LIB) $(OBJECTS)

#
# The following target has been added to expand certain file names,
# that get truncated to 12 characters by rcs, to their original
# 14 character file names.
#
expand:
	-/bin/cp extract_nldt extract_nldt.c
	-/bin/cp format_nldt. format_nldt.c
	-/bin/cp nlconvclock. nlconvclock.c
	-/bin/cp nlconvcustda nlconvcustda.c
	-/bin/cp nlfmtcustdat nlfmtcustdat.c
	-/bin/cp nlfmtlongcal nlfmtlongcal.c
	-/bin/cp nlkeycompare nlkeycompare.c
	-/bin/cp nlswitchbuf. nlswitchbuf.c
	-/bin/cp nltranslate. nltranslate.c


#
# For completeness sake, go ahead and nuke all the "expanded" source
# files.
#
clean:
	/bin/rm -f $(OBJECTS) $(VERSION) extract_nldt.c format_nldt.c \
		nlconvclock.c nlconvcustda.c nlfmtcustdat.c nlfmtlongcal.c \
		nlkeycompare.c nlswitchbuf.c nltranslate.c

#
# For the clobber target a rm -rf was added to delete the nls directory and
# everything that it contained.  Previously just the .cat files were removed
# in the clean target.
#
clobber: clean
	/bin/rm -rf $(LIB) $(NLSDIR)

catalogs: 
	test ! -d $(NLSDIR) && mkdir $(NLSDIR);			\
	for i in $(LANGS);					\
		do 						\
			test ! -d $(NLSDIR)/$$i && mkdir $(NLSDIR)/$$i; \
			$(GENCAT) $(NLSDIR)/$$i/custdat.cat c.$$i; \
		done

.c.o:
	$(CC) $(CFLAGS) $(PFLAGS) -c $<
# copy /usr/lib/libpnls to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! cp libportnls.a ../../../../Filesets.sl/BUILDENV/usr/lib/libportnls.a
	! chmog 0444 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/libportnls.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! cp libportnls.a ../../../../Filesets.sl/PROG-AUX/usr/lib/libportnls.a
	! chmog 0444 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/libportnls.a
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! cp nls/n-computer/custdat.cat ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset AMERICAN
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american
	! cp nls/american/custdat.cat ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset ARABICW
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ARABICW/usr/lib/nls/arabic-w
	! cp nls/arabic-w/custdat.cat ../../../../Filesets.sl/ARABICW/usr/lib/nls/arabic-w/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/ARABICW/usr/lib/nls/arabic-w/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/usr/lib/nls/arabic-w
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset ARABIC
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic
	! cp nls/arabic/custdat.cat ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset CFRENCH
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french
	! cp nls/c-french/custdat.cat ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset CHINESES
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CHINESES/usr/lib/nls/chinese-s
	! cp nls/chinese-s/custdat.cat ../../../../Filesets.sl/CHINESES/usr/lib/nls/chinese-s/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CHINESES/usr/lib/nls/chinese-s/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/usr/lib/nls/chinese-s
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset CHINESET
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t
	! cp nls/chinese-t/custdat.cat ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset DANISH
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DANISH/usr/lib/nls/danish
	! cp nls/danish/custdat.cat ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/danish
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset DUTCH
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch
	! cp nls/dutch/custdat.cat ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset ENGLISH
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english
	! cp nls/english/custdat.cat ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset FINNISH
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish
	! cp nls/finnish/custdat.cat ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset FRENCH
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/FRENCH/usr/lib/nls/french
	! cp nls/french/custdat.cat ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/french
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset GERMAN
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/GERMAN/usr/lib/nls/german
	! cp nls/german/custdat.cat ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/german
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset GREEK
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/GREEK/usr/lib/nls/greek
	! cp nls/greek/custdat.cat ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/greek
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset HEBREW
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew
	! cp nls/hebrew/custdat.cat ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset ICELANDIC
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic
	! cp nls/icelandic/custdat.cat ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset ITALIAN
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian
	! cp nls/italian/custdat.cat ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset JAPANESE
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese
	! cp nls/japanese/custdat.cat ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset KATAKANA
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/KATAKANA/usr/lib/nls/katakana
	! cp nls/katakana/custdat.cat ../../../../Filesets.sl/KATAKANA/usr/lib/nls/katakana/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/KATAKANA/usr/lib/nls/katakana/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/usr/lib/nls/katakana
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset KOREAN
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/KOREAN/usr/lib/nls/korean
	! cp nls/korean/custdat.cat ../../../../Filesets.sl/KOREAN/usr/lib/nls/korean/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/KOREAN/usr/lib/nls/korean/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/usr/lib/nls/korean
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset NORWEGIAN
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian
	! cp nls/norwegian/custdat.cat ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset PORTUGUES
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese
	! cp nls/portuguese/custdat.cat ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset SPANISH
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish
	! cp nls/spanish/custdat.cat ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset SWEDISH
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish
	! cp nls/swedish/custdat.cat ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libpnls to the . relative fileset TURKISH
# change mode, owner, and group on /usr/lib/libpnls
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish
	! cp nls/turkish/custdat.cat ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/custdat.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/custdat.cat
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
