#
# @(#) $Revision: 70.7.1.81 $
#
# Makefile for the Native Language Support config and environment files
#

LANGS	= american arabic arabic-w \
	  c-french chinese-s chinese-t \
	  danish dutch \
	  english \
	  finnish french \
	  german greek \
	  hebrew \
	  icelandic italian \
	  japanese \
	  katakana korean \
	  norwegian \
	  portuguese \
	  spanish swedish \
	  turkish \
	  euc \
	  american.iso88591 \
	  c-french.iso88591 \
	  danish.iso88591 \
	  dutch.iso88591 \
	  english.iso88591 \
	  finnish.iso88591 \
	  french.iso88591 \
	  german.iso88591 \
	  italian.iso88591 \
	  norwegian.iso88591 \
	  portuguese.iso88591 \
	  spanish.iso88591 \
	  swedish.iso88591 \
	  icelandic.iso88591\
	  czech\
	  hungarian\
	  polish\
	  rumanian\
	  serbocroatian\
	  slovene\
	  russian\
	  bulgarian\
	  arabic.iso88596\
		 greek.iso88597\
		  hebrew.iso88598\
		  turkish.iso88599\
		  thai\
		  chinese-t.big5

BLDDIR	= ../../bin/buildlang
BLDTEMP	= ./BLDTEMP
BLDLANG = $(BLDTEMP)/buildlang

# *********************** start old build *****************************

OLDNLS	= \
    collate8      collate8.cfra collate8.dan  collate8.dtch \
    collate8.eng  collate8.fin  collate8.fra  collate8.ger  \
    collate8.ice  collate8.ita  collate8.nor  collate8.port \
    collate8.spa  collate8.swe  collate8.kata collate8.ara  \
    collate8.araw collate8.grk  collate8.heb  collate8.tur  \
    collate8.chs  collate8.cht  collate8.jpn  collate8.krn
CFLAGS	=  
CC	= cc
ARCHIVE = -Wl,-a,archive

# ************************ end old build ******************************

default: config $(LANGS) $(OLDNLS)

all: default

config: D.config
	cp D.config $@

#
# The intent here is to build a copy of buildlang that will run on the
# build machine now but will generate locale.def files for the target
# machine.  We can't use the buildlang currently installed on the build
# machine because buildlang may have been modified to generate the
# locale.def files in a different format for the new release we are
# trying to build.
#
$(BLDLANG): 
	rm -rf $(BLDTEMP)
	mkdir $(BLDTEMP)
	cp $(BLDDIR)/* $(BLDTEMP)
	cd $(BLDTEMP); make -k clobber
	cd $(BLDTEMP); make -k LDFLAGS=$(ARCHIVE)
	cd $(BLDTEMP); make -k clean

american: $(BLDLANG) D.american
	$(BLDLANG) -n D.american
	mv locale.def $@

arabic: $(BLDLANG) D.arabic
	$(BLDLANG) -n D.arabic
	mv locale.def $@

arabic-w: $(BLDLANG) D.arabic-w
	$(BLDLANG) -n D.arabic-w
	mv locale.def $@

c-french: $(BLDLANG) D.c-french
	$(BLDLANG) -n D.c-french
	mv locale.def $@

chinese-s: $(BLDLANG) D.chinese-s
	$(BLDLANG) -n D.chinese-s
	mv locale.def $@

chinese-t: $(BLDLANG) D.chinese-t
	$(BLDLANG) -n D.chinese-t
	mv locale.def $@

danish: $(BLDLANG) D.danish
	$(BLDLANG) -n D.danish
	mv locale.def $@

dutch: $(BLDLANG) D.dutch
	$(BLDLANG) -n D.dutch
	mv locale.def $@

english: $(BLDLANG) D.english
	$(BLDLANG) -n D.english
	mv locale.def $@

finnish: $(BLDLANG) D.finnish
	$(BLDLANG) -n D.finnish
	mv locale.def $@

french: $(BLDLANG) D.french
	$(BLDLANG) -n D.french
	mv locale.def $@

german: $(BLDLANG) D.german
	$(BLDLANG) -n D.german
	mv locale.def $@

greek: $(BLDLANG) D.greek
	$(BLDLANG) -n D.greek
	mv locale.def $@

hebrew: $(BLDLANG) D.hebrew
	$(BLDLANG) -n D.hebrew
	mv locale.def $@

icelandic: $(BLDLANG) D.icelandic
	$(BLDLANG) -n D.icelandic
	mv locale.def $@

italian: $(BLDLANG) D.italian
	$(BLDLANG) -n D.italian
	mv locale.def $@

japanese: $(BLDLANG) D.japanese W.japanese
	$(BLDLANG) -n D.japanese
	mv locale.def $@
	uudecode W.japanese

katakana: $(BLDLANG) D.katakana
	$(BLDLANG) -n D.katakana
	mv locale.def $@

korean: $(BLDLANG) D.korean
	$(BLDLANG) -n D.korean
	mv locale.def $@

norwegian: $(BLDLANG) D.norwegian
	$(BLDLANG) -n D.norwegian
	mv locale.def $@

portuguese: $(BLDLANG) D.portuguese
	$(BLDLANG) -n D.portuguese
	mv locale.def $@

spanish: $(BLDLANG) D.spanish
	$(BLDLANG) -n D.spanish
	mv locale.def $@

swedish: $(BLDLANG) D.swedish
	$(BLDLANG) -n D.swedish
	mv locale.def $@

turkish: $(BLDLANG) D.turkish
	$(BLDLANG) -n D.turkish
	mv locale.def $@

euc: $(BLDLANG) D.japanese.euc W.japanese.euc
	$(BLDLANG) -n D.japanese.euc
	mv locale.def $@
	uudecode W.japanese.euc

american.iso88591: $(BLDLANG) D.american.iso88591
	$(BLDLANG) -n D.american.iso88591
	mv locale.def $@

c-french.iso88591: $(BLDLANG) D.c-french.iso88591
	$(BLDLANG) -n D.c-french.iso88591
	mv locale.def $@

danish.iso88591: $(BLDLANG) D.danish.iso88591
	$(BLDLANG) -n D.danish.iso88591
	mv locale.def $@

dutch.iso88591: $(BLDLANG) D.dutch.iso88591
	$(BLDLANG) -n D.dutch.iso88591
	mv locale.def $@

english.iso88591: $(BLDLANG) D.english.iso88591
	$(BLDLANG) -n D.english.iso88591
	mv locale.def $@

finnish.iso88591: $(BLDLANG) D.finnish.iso88591
	$(BLDLANG) -n D.finnish.iso88591
	mv locale.def $@

french.iso88591: $(BLDLANG) D.french.iso88591
	$(BLDLANG) -n D.french.iso88591
	mv locale.def $@

german.iso88591: $(BLDLANG) D.german.iso88591
	$(BLDLANG) -n D.german.iso88591
	mv locale.def $@

italian.iso88591: $(BLDLANG) D.italian.iso88591
	$(BLDLANG) -n D.italian.iso88591
	mv locale.def $@

norwegian.iso88591: $(BLDLANG) D.norwegian.iso88591
	$(BLDLANG) -n D.norwegian.iso88591
	mv locale.def $@

portuguese.iso88591: $(BLDLANG) D.portuguese.iso88591
	$(BLDLANG) -n D.portuguese.iso88591
	mv locale.def $@

spanish.iso88591: $(BLDLANG) D.spanish.iso88591
	$(BLDLANG) -n D.spanish.iso88591
	mv locale.def $@

swedish.iso88591: $(BLDLANG) D.swedish.iso88591
	$(BLDLANG) -n D.swedish.iso88591
	mv locale.def $@

icelandic.iso88591: $(BLDLANG) D.icelandic.iso88591
	$(BLDLANG) -n D.icelandic.iso88591
	mv locale.def $@

czech: $(BLDLANG) D.czech
	$(BLDLANG) -n D.czech
	mv locale.def $@

hungarian: $(BLDLANG) D.hungarian
	$(BLDLANG) -n D.hungarian
	mv locale.def $@

polish: $(BLDLANG) D.polish
	$(BLDLANG) -n D.polish
	mv locale.def $@

rumanian: $(BLDLANG) D.rumanian
	$(BLDLANG) -n D.rumanian
	mv locale.def $@

serbocroatian: $(BLDLANG) D.serbocroatian
	$(BLDLANG) -n D.serbocroatian
	mv locale.def $@

slovene: $(BLDLANG) D.slovene
	$(BLDLANG) -n D.slovene
	mv locale.def $@

russian: $(BLDLANG) D.russian
	$(BLDLANG) -n D.russian
	mv locale.def $@

bulgarian: $(BLDLANG) D.bulgarian
	$(BLDLANG) -n D.bulgarian
	mv locale.def $@

arabic.iso88596: $(BLDLANG) D.arabic.iso88596
	$(BLDLANG) -n D.arabic.iso88596
	mv locale.def $@

greek.iso88597: $(BLDLANG)  D.greek.iso88597
	$(BLDLANG) -n D.greek.iso88597
	mv locale.def $@

hebrew.iso88598:        $(BLDLANG)  D.hebrew.iso88598
	$(BLDLANG) -n D.hebrew.iso88598
	mv locale.def $@

turkish.iso88599:       $(BLDLANG)  D.turkish.iso88599
	$(BLDLANG) -n D.turkish.iso88599
	mv locale.def $@

thai:   $(BLDLANG)  D.thai
	$(BLDLANG) -n D.thai
	mv locale.def $@

chinese-t.big5:        $(BLDLANG)  D.chinese-t.big5
	$(BLDLANG) -n D.chinese-t.big5    
	mv locale.def $@

# *********************** start old build *****************************

collate8: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DAMERICAN -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.cfra: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DCFRENCH -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.dan: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DDANISH -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.dtch: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DDUTCH -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.eng: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DENGLISH -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.fin: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DFINNISH -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.fra: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DFRENCH -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.ger: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DGERMAN -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.ice: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DICELANDIC -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.ita: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DITALIAN -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.nor: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DNORWEGIAN -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.port: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DPORTUGUESE -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.spa: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DSPANISH -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.swe: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DSWEDISH -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.kata: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DKATAKANA -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.ara: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DARABIC -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.araw: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DARABICW -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.grk: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DGREEK -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.heb: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DHEBREW -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.tur: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DTURKISH -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.chs: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DCHINESES -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.cht: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DCHINESET -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.jpn: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DJAPANESE -o coll8tab coll8tab.c
	./coll8tab >$@

collate8.krn: coll8tab.c 
	$(CC) $(CFLAGS) $(ARCHIVE) -DKOREAN -o coll8tab coll8tab.c
	./coll8tab >$@

# ************************ end old build ******************************

clean: 
	rm -rf *.o $(BLDLANG) $(BLDTEMP) locale.def ctypetab shifttab \
		coll8tab infomsg info.msg

clobber: clean
	rm -f $(LANGS) $(OLDNLS) config euc.ext japanese.ext
# copy /usr/lib/nlsdef to the . relative fileset AMERICAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american
	! cp collate8 ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/lib/nls
	! cp config ../../../../Filesets.sl/UX-CORE/usr/lib/nls/config
	! chmog 0444 bin bin ../../../../Filesets.sl/UX-CORE/usr/lib/nls/config
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/lib/nls
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset AMERICAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/iso88591
	! cp american.iso88591 ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/iso88591/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/iso88591/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset AMERICAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american
	! cp american ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset ARABICW
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ARABICW/usr/lib/nls/arabic-w
	! cp collate8.araw ../../../../Filesets.sl/ARABICW/usr/lib/nls/arabic-w/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/ARABICW/usr/lib/nls/arabic-w/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/usr/lib/nls/arabic-w
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset ARABICW
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ARABICW/usr/lib/nls/arabic-w
	! cp arabic-w ../../../../Filesets.sl/ARABICW/usr/lib/nls/arabic-w/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/ARABICW/usr/lib/nls/arabic-w/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/usr/lib/nls/arabic-w
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset ARABIC
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic
	! cp collate8.ara ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset ARABIC
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/iso88596
	! cp arabic.iso88596 ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/iso88596/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/iso88596/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/iso88596
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset ARABIC
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic
	! cp arabic ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset BULGARIAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BULGARIAN/usr/lib/nls/bulgarian
	! cp bulgarian ../../../../Filesets.sl/BULGARIAN/usr/lib/nls/bulgarian/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/BULGARIAN/usr/lib/nls/bulgarian/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/BULGARIAN/usr/lib/nls/bulgarian
	! chmog 555 bin bin ../../../../Filesets.sl/BULGARIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/BULGARIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/BULGARIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BULGARIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset CFRENCH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french
	! cp collate8.cfra ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset CFRENCH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/iso88591
	! cp c-french.iso88591 ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/iso88591/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/iso88591/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset CFRENCH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french
	! cp c-french ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset CHINESES
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CHINESES/usr/lib/nls/chinese-s
	! cp collate8.chs ../../../../Filesets.sl/CHINESES/usr/lib/nls/chinese-s/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/CHINESES/usr/lib/nls/chinese-s/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/usr/lib/nls/chinese-s
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset CHINESES
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CHINESES/usr/lib/nls/chinese-s
	! cp chinese-s ../../../../Filesets.sl/CHINESES/usr/lib/nls/chinese-s/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/CHINESES/usr/lib/nls/chinese-s/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/usr/lib/nls/chinese-s
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset CHINESET
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/big5
	! cp chinese-t.big5 ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/big5/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/big5/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/big5
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset CHINESET
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t
	! cp collate8.cht ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset CHINESET
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t
	! cp chinese-t ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset CZECH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CZECH/usr/lib/nls/czech
	! cp czech ../../../../Filesets.sl/CZECH/usr/lib/nls/czech/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/CZECH/usr/lib/nls/czech/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/CZECH/usr/lib/nls/czech
	! chmog 555 bin bin ../../../../Filesets.sl/CZECH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CZECH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CZECH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CZECH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset DANISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DANISH/usr/lib/nls/danish
	! cp collate8.dan ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/danish
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset DANISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/iso88591
	! cp danish.iso88591 ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/iso88591/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/iso88591/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset DANISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DANISH/usr/lib/nls/danish
	! cp danish ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/danish
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset DUTCH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch
	! cp collate8.dtch ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset DUTCH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/iso88591
	! cp dutch.iso88591 ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/iso88591/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/iso88591/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset DUTCH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch
	! cp dutch ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset ENGLISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english
	! cp collate8.eng ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset ENGLISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/iso88591
	! cp english.iso88591 ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/iso88591/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/iso88591/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset ENGLISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english
	! cp english ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset FINNISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish
	! cp collate8.fin ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset FINNISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/iso88591
	! cp finnish.iso88591 ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/iso88591/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/iso88591/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset FINNISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish
	! cp finnish ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset FRENCH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/FRENCH/usr/lib/nls/french
	! cp collate8.fra ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/french
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset FRENCH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/iso88591
	! cp french.iso88591 ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/iso88591/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/iso88591/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset FRENCH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/FRENCH/usr/lib/nls/french
	! cp french ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/french
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset GERMAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/GERMAN/usr/lib/nls/german
	! cp collate8.ger ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/german
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset GERMAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/iso88591
	! cp german.iso88591 ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/iso88591/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/iso88591/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset GERMAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/GERMAN/usr/lib/nls/german
	! cp german ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/german
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset GREEK
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/GREEK/usr/lib/nls/greek
	! cp collate8.grk ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/greek
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset GREEK
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/iso88597
	! cp greek.iso88597 ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/iso88597/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/iso88597/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/iso88597
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset GREEK
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/GREEK/usr/lib/nls/greek
	! cp greek ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/greek
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset HEBREW
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew
	! cp collate8.heb ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset HEBREW
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/iso88598
	! cp hebrew.iso88598 ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/iso88598/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/iso88598/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/iso88598
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset HEBREW
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew
	! cp hebrew ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset HUNGARIAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/HUNGARIAN/usr/lib/nls/hungarian
	! cp hungarian ../../../../Filesets.sl/HUNGARIAN/usr/lib/nls/hungarian/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/HUNGARIAN/usr/lib/nls/hungarian/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/HUNGARIAN/usr/lib/nls/hungarian
	! chmog 555 bin bin ../../../../Filesets.sl/HUNGARIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/HUNGARIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/HUNGARIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/HUNGARIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset ICELANDIC
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic
	! cp collate8.ice ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset ICELANDIC
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/iso88591
	! cp icelandic.iso88591 ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/iso88591/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/iso88591/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset ICELANDIC
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic
	! cp icelandic ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset ITALIAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian
	! cp collate8.ita ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset ITALIAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/iso88591
	! cp italian.iso88591 ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/iso88591/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/iso88591/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset ITALIAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian
	! cp italian ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset JAPANESE
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese
	! cp collate8.jpn ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset JAPANESE
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/euc
	! cp euc ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/euc/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/euc/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/euc
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset JAPANESE
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/euc
	! cp euc.ext ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/euc/locale.ext
	! chmog 0444 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/euc/locale.ext
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/euc
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset JAPANESE
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese
	! cp japanese ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset JAPANESE
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese
	! cp japanese.ext ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/locale.ext
	! chmog 0444 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/locale.ext
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset KATAKANA
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/KATAKANA/usr/lib/nls/katakana
	! cp collate8.kata ../../../../Filesets.sl/KATAKANA/usr/lib/nls/katakana/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/KATAKANA/usr/lib/nls/katakana/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/usr/lib/nls/katakana
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset KATAKANA
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/KATAKANA/usr/lib/nls/katakana
	! cp katakana ../../../../Filesets.sl/KATAKANA/usr/lib/nls/katakana/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/KATAKANA/usr/lib/nls/katakana/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/usr/lib/nls/katakana
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset KOREAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/KOREAN/usr/lib/nls/korean
	! cp collate8.krn ../../../../Filesets.sl/KOREAN/usr/lib/nls/korean/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/KOREAN/usr/lib/nls/korean/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/usr/lib/nls/korean
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset KOREAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/KOREAN/usr/lib/nls/korean
	! cp korean ../../../../Filesets.sl/KOREAN/usr/lib/nls/korean/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/KOREAN/usr/lib/nls/korean/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/usr/lib/nls/korean
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset NORWEGIAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian
	! cp collate8.nor ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset NORWEGIAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/iso88591
	! cp norwegian.iso88591 ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/iso88591/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/iso88591/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset NORWEGIAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian
	! cp norwegian ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset POLISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/POLISH/usr/lib/nls/polish
	! cp polish ../../../../Filesets.sl/POLISH/usr/lib/nls/polish/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/POLISH/usr/lib/nls/polish/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/POLISH/usr/lib/nls/polish
	! chmog 555 bin bin ../../../../Filesets.sl/POLISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/POLISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/POLISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/POLISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset PORTUGUES
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese
	! cp collate8.port ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset PORTUGUES
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/iso88591
	! cp portuguese.iso88591 ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/iso88591/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/iso88591/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset PORTUGUES
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese
	! cp portuguese ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset RUMANIAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/RUMANIAN/usr/lib/nls/rumanian
	! cp rumanian ../../../../Filesets.sl/RUMANIAN/usr/lib/nls/rumanian/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/RUMANIAN/usr/lib/nls/rumanian/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/RUMANIAN/usr/lib/nls/rumanian
	! chmog 555 bin bin ../../../../Filesets.sl/RUMANIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/RUMANIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/RUMANIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/RUMANIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset RUSSIAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/RUSSIAN/usr/lib/nls/russian
	! cp russian ../../../../Filesets.sl/RUSSIAN/usr/lib/nls/russian/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/RUSSIAN/usr/lib/nls/russian/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/RUSSIAN/usr/lib/nls/russian
	! chmog 555 bin bin ../../../../Filesets.sl/RUSSIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/RUSSIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/RUSSIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/RUSSIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset SERBOCROATIAN
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SERBOCROATIAN/usr/lib/nls/serbocroatian
	! cp serbocroatian ../../../../Filesets.sl/SERBOCROATIAN/usr/lib/nls/serbocroatian/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/SERBOCROATIAN/usr/lib/nls/serbocroatian/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/SERBOCROATIAN/usr/lib/nls/serbocroatian
	! chmog 555 bin bin ../../../../Filesets.sl/SERBOCROATIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SERBOCROATIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SERBOCROATIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SERBOCROATIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset SLOVENE
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SLOVENE/usr/lib/nls/slovene
	! cp slovene ../../../../Filesets.sl/SLOVENE/usr/lib/nls/slovene/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/SLOVENE/usr/lib/nls/slovene/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/SLOVENE/usr/lib/nls/slovene
	! chmog 555 bin bin ../../../../Filesets.sl/SLOVENE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SLOVENE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SLOVENE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SLOVENE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset SPANISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish
	! cp collate8.spa ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset SPANISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/iso88591
	! cp spanish.iso88591 ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/iso88591/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/iso88591/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset SPANISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish
	! cp spanish ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset SWEDISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish
	! cp collate8.swe ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset SWEDISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/iso88591
	! cp swedish.iso88591 ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/iso88591/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/iso88591/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset SWEDISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish
	! cp swedish ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset THAI
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/THAI/usr/lib/nls/thai
	! cp thai ../../../../Filesets.sl/THAI/usr/lib/nls/thai/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/THAI/usr/lib/nls/thai/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/THAI/usr/lib/nls/thai
	! chmog 555 bin bin ../../../../Filesets.sl/THAI/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/THAI/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/THAI/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/THAI/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset TURKISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish
	! cp collate8.tur ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/collate8
	! chmog 0444 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/collate8
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset TURKISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/iso88599
	! cp turkish.iso88599 ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/iso88599/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/iso88599/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/iso88599
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsdef to the . relative fileset TURKISH
# change mode, owner, and group on /usr/lib/nlsdef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish
	! cp turkish ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/locale.def
	! chmog 0444 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/locale.def
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
