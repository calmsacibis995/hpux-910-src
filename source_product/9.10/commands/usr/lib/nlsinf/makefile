#
# @(#) $Revision: 70.14.1.54 $
#
# Makefile for the Native Language Support config and environment files
#

LANGS	= american arabic arabic-w  \
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
	  hungarian	  \
	  polish\
	  rumanian\
	  serbocroatian 	\
	  slovene	\
	  russian\
	  bulgarian\
	  \
	greek.iso88597\
	hebrew.iso88598\
	turkish.iso88599\
	arabic.iso88596\
	thai\
	chinese-t.big5

LDEFDIR	= ../../bin/localedef
LDEFTEMP = ./LDEFTEMP
LOCALEDEF = $(LDEFTEMP)/localedef

CFLAGS	=  
CC	= cc
ARCHIVE = -Wl,-a,archive

# ************************ end old build ******************************

default: config $(LANGS) $(OLDNLS)

all: default

#Using the same config file that is used by buildlang
config: ../nlsdef/D.config
	cp ../nlsdef/D.config $@

#
# The intent here is to build a copy of localedef that will run on the
# build machine now but will generate locale.inf files for the target
# machine.  We can't use the buildlang currently installed on the build
# machine because localedef may have been modified to generate the
# locale.inf files in a different format for the new release we are
# trying to builI.
#
$(LOCALEDEF): 
	rm -rf $(LDEFTEMP)
	mkdir $(LDEFTEMP)
	cp $(LDEFDIR)/* $(LDEFTEMP)
	cd $(LDEFTEMP); make -k clobber
	cd $(LDEFTEMP); make -k LDFLAGS=$(ARCHIVE)
	cd $(LDEFTEMP); make -k clean

american: $(LOCALEDEF) I.american
	$(LOCALEDEF) -n -i I.american
	mv locale.inf $@

arabic: $(LOCALEDEF) I.arabic
	$(LOCALEDEF) -n -i I.arabic
	mv locale.inf $@

arabic-w: $(LOCALEDEF) I.arabic-w
	$(LOCALEDEF) -n -i I.arabic-w
	mv locale.inf $@

c-french: $(LOCALEDEF) I.c-french
	$(LOCALEDEF) -n -i I.c-french
	mv locale.inf $@

chinese-s: $(LOCALEDEF) I.chinese-s
	$(LOCALEDEF) -n -i I.chinese-s
	mv locale.inf $@

chinese-t: $(LOCALEDEF) I.chinese-t
	$(LOCALEDEF) -n -i I.chinese-t
	mv locale.inf $@

danish: $(LOCALEDEF) I.danish
	$(LOCALEDEF) -n -i I.danish
	mv locale.inf $@

dutch: $(LOCALEDEF) I.dutch
	$(LOCALEDEF) -n -i I.dutch
	mv locale.inf $@

english: $(LOCALEDEF) I.english
	$(LOCALEDEF) -n -i I.english
	mv locale.inf $@

finnish: $(LOCALEDEF) I.finnish
	$(LOCALEDEF) -n -i I.finnish
	mv locale.inf $@

french: $(LOCALEDEF) I.french
	$(LOCALEDEF) -n -i I.french
	mv locale.inf $@

german: $(LOCALEDEF) I.german
	$(LOCALEDEF) -n -i I.german
	mv locale.inf $@

greek: $(LOCALEDEF) I.greek
	$(LOCALEDEF) -n -i I.greek
	mv locale.inf $@

hebrew: $(LOCALEDEF) I.hebrew
	$(LOCALEDEF) -n -i I.hebrew
	mv locale.inf $@

icelandic: $(LOCALEDEF) I.icelandic
	$(LOCALEDEF) -n -i I.icelandic
	mv locale.inf $@

italian: $(LOCALEDEF) I.italian
	$(LOCALEDEF) -n -i I.italian
	mv locale.inf $@

japanese: $(LOCALEDEF) I.japanese 
	$(LOCALEDEF) -n -i I.japanese
	mv locale.inf $@

katakana: $(LOCALEDEF) I.katakana
	$(LOCALEDEF) -n -i I.katakana
	mv locale.inf $@

korean: $(LOCALEDEF) I.korean
	$(LOCALEDEF) -n -i I.korean
	mv locale.inf $@

norwegian: $(LOCALEDEF) I.norwegian
	$(LOCALEDEF) -n -i I.norwegian
	mv locale.inf $@

portuguese: $(LOCALEDEF) I.portuguese
	$(LOCALEDEF) -n -i I.portuguese
	mv locale.inf $@

spanish: $(LOCALEDEF) I.spanish
	$(LOCALEDEF) -n -i I.spanish
	mv locale.inf $@

swedish: $(LOCALEDEF) I.swedish
	$(LOCALEDEF) -n -i I.swedish
	mv locale.inf $@

turkish: $(LOCALEDEF) I.turkish
	$(LOCALEDEF) -n -i I.turkish
	mv locale.inf $@

euc: $(LOCALEDEF) I.japanese.euc 
	$(LOCALEDEF) -n -i I.japanese.euc
	mv locale.inf $@

american.iso88591: $(LOCALEDEF) I.american.iso88591
	$(LOCALEDEF) -n -i I.american.iso88591
	mv locale.inf $@

c-french.iso88591: $(LOCALEDEF) I.c-french.iso88591
	$(LOCALEDEF) -n -i I.c-french.iso88591
	mv locale.inf $@

danish.iso88591: $(LOCALEDEF) I.danish.iso88591
	$(LOCALEDEF) -n -i I.danish.iso88591
	mv locale.inf $@

dutch.iso88591: $(LOCALEDEF) I.dutch.iso88591
	$(LOCALEDEF) -n -i I.dutch.iso88591
	mv locale.inf $@

english.iso88591: $(LOCALEDEF) I.english.iso88591
	$(LOCALEDEF) -n -i I.english.iso88591
	mv locale.inf $@

finnish.iso88591: $(LOCALEDEF) I.finnish.iso88591
	$(LOCALEDEF) -n -i I.finnish.iso88591
	mv locale.inf $@

french.iso88591: $(LOCALEDEF) I.french.iso88591
	$(LOCALEDEF) -n -i I.french.iso88591
	mv locale.inf $@

german.iso88591: $(LOCALEDEF) I.german.iso88591
	$(LOCALEDEF) -n -i I.german.iso88591
	mv locale.inf $@

italian.iso88591: $(LOCALEDEF) I.italian.iso88591
	$(LOCALEDEF) -n -i I.italian.iso88591
	mv locale.inf $@

norwegian.iso88591: $(LOCALEDEF) I.norwegian.iso88591
	$(LOCALEDEF) -n -i I.norwegian.iso88591
	mv locale.inf $@

portuguese.iso88591: $(LOCALEDEF) I.portuguese.iso88591
	$(LOCALEDEF) -n -i I.portuguese.iso88591
	mv locale.inf $@

spanish.iso88591: $(LOCALEDEF) I.spanish.iso88591
	$(LOCALEDEF) -n -i I.spanish.iso88591
	mv locale.inf $@

swedish.iso88591: $(LOCALEDEF) I.swedish.iso88591
	$(LOCALEDEF) -n -i I.swedish.iso88591
	mv locale.inf $@

icelandic.iso88591: $(LOCALEDEF) I.icelandic.iso88591
	$(LOCALEDEF) -n -i I.icelandic.iso88591
	mv locale.inf $@

czech: $(LOCALEDEF) I.czech
	$(LOCALEDEF) -n -i I.czech
	mv locale.inf $@

hungarian: $(LOCALEDEF) I.hungarian
	$(LOCALEDEF) -n -i I.hungarian
	mv locale.inf $@

polish: $(LOCALEDEF) I.polish
	$(LOCALEDEF) -n -i I.polish
	mv locale.inf $@

rumanian: $(LOCALEDEF) I.rumanian
	$(LOCALEDEF) -n -i I.rumanian
	mv locale.inf $@

serbocroatian: $(LOCALEDEF) I.serbocroatian
	$(LOCALEDEF) -n -i I.serbocroatian
	mv locale.inf $@

slovene: $(LOCALEDEF) I.slovene
	$(LOCALEDEF) -n -i I.slovene
	mv locale.inf $@

russian: $(LOCALEDEF) I.russian
	$(LOCALEDEF) -n -i I.russian
	mv locale.inf $@

bulgarian: $(LOCALEDEF) I.bulgarian
	$(LOCALEDEF) -n -i I.bulgarian
	mv locale.inf $@

arabic.iso88596: $(LOCALEDEF) I.arabic.iso88596
	$(LOCALEDEF) -n -i I.arabic.iso88596
	mv locale.inf $@

greek.iso88597:	$(LOCALEDEF)  I.greek.iso88597
	$(LOCALEDEF) -n -i I.greek.iso88597
	mv locale.inf $@

hebrew.iso88598:	$(LOCALEDEF)  I.hebrew.iso88598
	$(LOCALEDEF) -n -i I.hebrew.iso88598
	mv locale.inf $@

turkish.iso88599:	$(LOCALEDEF)  I.turkish.iso88599
	$(LOCALEDEF) -n -i I.turkish.iso88599
	mv locale.inf $@

thai:	$(LOCALEDEF)  I.thai
	$(LOCALEDEF) -n -i I.thai
	mv locale.inf $@

chinese-t.big5:	$(LOCALEDEF)  I.chinese-t.big5
	$(LOCALEDEF) -n -i I.chinese-t.big5
	mv locale.inf $@



clean: 
	rm -rf *.o $(LOCALEDEF) $(LDEFTEMP) locale.inf \
		 infomsg info.msg

clobber: clean
	rm -f $(LANGS) $(OLDNLS) config 
# copy /usr/lib/nlsinf to the . relative fileset AMERICAN
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/iso88591
	! cp american.iso88591 ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/iso88591/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/iso88591/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset AMERICAN
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american
	! cp american ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/american
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/AMERICAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset ARABICW
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ARABICW/usr/lib/nls/arabic-w
	! cp arabic-w ../../../../Filesets.sl/ARABICW/usr/lib/nls/arabic-w/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/ARABICW/usr/lib/nls/arabic-w/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/usr/lib/nls/arabic-w
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABICW/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset ARABIC
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/iso88596
	! cp arabic.iso88596 ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/iso88596/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/iso88596/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/iso88596
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset ARABIC
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic
	! cp arabic ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/arabic
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ARABIC/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset BULGARIAN
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/BULGARIAN/usr/lib/nls/bulgarian
	! cp bulgarian ../../../../Filesets.sl/BULGARIAN/usr/lib/nls/bulgarian/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/BULGARIAN/usr/lib/nls/bulgarian/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/BULGARIAN/usr/lib/nls/bulgarian
	! chmog 555 bin bin ../../../../Filesets.sl/BULGARIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/BULGARIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/BULGARIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BULGARIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset CFRENCH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/iso88591
	! cp c-french.iso88591 ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/iso88591/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/iso88591/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset CFRENCH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french
	! cp c-french ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/c-french
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CFRENCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset CHINESES
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CHINESES/usr/lib/nls/chinese-s
	! cp chinese-s ../../../../Filesets.sl/CHINESES/usr/lib/nls/chinese-s/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/CHINESES/usr/lib/nls/chinese-s/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/usr/lib/nls/chinese-s
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESES/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset CHINESET
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/big5
	! cp chinese-t.big5 ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/big5/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/big5/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/big5
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset CHINESET
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t
	! cp chinese-t ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/chinese-t
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CHINESET/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset CZECH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CZECH/usr/lib/nls/czech
	! cp czech ../../../../Filesets.sl/CZECH/usr/lib/nls/czech/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/CZECH/usr/lib/nls/czech/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/CZECH/usr/lib/nls/czech
	! chmog 555 bin bin ../../../../Filesets.sl/CZECH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CZECH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CZECH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CZECH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset DANISH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/iso88591
	! cp danish.iso88591 ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/iso88591/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/iso88591/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset DANISH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DANISH/usr/lib/nls/danish
	! cp danish ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/danish/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/danish
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DANISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset DUTCH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/iso88591
	! cp dutch.iso88591 ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/iso88591/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/iso88591/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset DUTCH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch
	! cp dutch ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/dutch
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DUTCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset ENGLISH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/iso88591
	! cp english.iso88591 ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/iso88591/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/iso88591/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset ENGLISH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english
	! cp english ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/english
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ENGLISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset FINNISH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/iso88591
	! cp finnish.iso88591 ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/iso88591/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/iso88591/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset FINNISH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish
	! cp finnish ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/finnish
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/FINNISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset FRENCH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/iso88591
	! cp french.iso88591 ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/iso88591/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/iso88591/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset FRENCH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/FRENCH/usr/lib/nls/french
	! cp french ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/french/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/french
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/FRENCH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset GERMAN
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/iso88591
	! cp german.iso88591 ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/iso88591/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/iso88591/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset GERMAN
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/GERMAN/usr/lib/nls/german
	! cp german ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/german/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/german
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/GERMAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset GREEK
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/iso88597
	! cp greek.iso88597 ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/iso88597/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/iso88597/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/iso88597
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset GREEK
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/GREEK/usr/lib/nls/greek
	! cp greek ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/greek/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/greek
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/GREEK/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset HEBREW
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/iso88598
	! cp hebrew.iso88598 ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/iso88598/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/iso88598/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/iso88598
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset HEBREW
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew
	! cp hebrew ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/hebrew
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/HEBREW/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset HUNGARIAN
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/HUNGARIAN/usr/lib/nls/hungarian
	! cp hungarian ../../../../Filesets.sl/HUNGARIAN/usr/lib/nls/hungarian/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/HUNGARIAN/usr/lib/nls/hungarian/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/HUNGARIAN/usr/lib/nls/hungarian
	! chmog 555 bin bin ../../../../Filesets.sl/HUNGARIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/HUNGARIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/HUNGARIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/HUNGARIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset ICELANDIC
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/iso88591
	! cp icelandic.iso88591 ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/iso88591/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/iso88591/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset ICELANDIC
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic
	! cp icelandic ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/icelandic
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ICELANDIC/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset ITALIAN
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/iso88591
	! cp italian.iso88591 ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/iso88591/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/iso88591/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset ITALIAN
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian
	! cp italian ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/italian
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/ITALIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset JAPANESE
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/euc
	! cp euc ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/euc/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/euc/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/euc
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset JAPANESE
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese
	! cp japanese ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/japanese
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/JAPANESE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset KATAKANA
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/KATAKANA/usr/lib/nls/katakana
	! cp katakana ../../../../Filesets.sl/KATAKANA/usr/lib/nls/katakana/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/KATAKANA/usr/lib/nls/katakana/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/usr/lib/nls/katakana
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/KATAKANA/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset KOREAN
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/KOREAN/usr/lib/nls/korean
	! cp korean ../../../../Filesets.sl/KOREAN/usr/lib/nls/korean/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/KOREAN/usr/lib/nls/korean/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/usr/lib/nls/korean
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/KOREAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset NORWEGIAN
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/iso88591
	! cp norwegian.iso88591 ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/iso88591/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/iso88591/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset NORWEGIAN
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian
	! cp norwegian ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/norwegian
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NORWEGIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset POLISH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/POLISH/usr/lib/nls/polish
	! cp polish ../../../../Filesets.sl/POLISH/usr/lib/nls/polish/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/POLISH/usr/lib/nls/polish/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/POLISH/usr/lib/nls/polish
	! chmog 555 bin bin ../../../../Filesets.sl/POLISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/POLISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/POLISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/POLISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset PORTUGUES
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/iso88591
	! cp portuguese.iso88591 ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/iso88591/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/iso88591/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset PORTUGUES
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese
	! cp portuguese ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/portuguese
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PORTUGUES/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset RUMANIAN
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/RUMANIAN/usr/lib/nls/rumanian
	! cp rumanian ../../../../Filesets.sl/RUMANIAN/usr/lib/nls/rumanian/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/RUMANIAN/usr/lib/nls/rumanian/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/RUMANIAN/usr/lib/nls/rumanian
	! chmog 555 bin bin ../../../../Filesets.sl/RUMANIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/RUMANIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/RUMANIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/RUMANIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset RUSSIAN
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/RUSSIAN/usr/lib/nls/russian
	! cp russian ../../../../Filesets.sl/RUSSIAN/usr/lib/nls/russian/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/RUSSIAN/usr/lib/nls/russian/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/RUSSIAN/usr/lib/nls/russian
	! chmog 555 bin bin ../../../../Filesets.sl/RUSSIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/RUSSIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/RUSSIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/RUSSIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset SERBOCROATIAN
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SERBOCROATIAN/usr/lib/nls/serbocroatian
	! cp serbocroatian ../../../../Filesets.sl/SERBOCROATIAN/usr/lib/nls/serbocroatian/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/SERBOCROATIAN/usr/lib/nls/serbocroatian/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/SERBOCROATIAN/usr/lib/nls/serbocroatian
	! chmog 555 bin bin ../../../../Filesets.sl/SERBOCROATIAN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SERBOCROATIAN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SERBOCROATIAN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SERBOCROATIAN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset SLOVENE
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SLOVENE/usr/lib/nls/slovene
	! cp slovene ../../../../Filesets.sl/SLOVENE/usr/lib/nls/slovene/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/SLOVENE/usr/lib/nls/slovene/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/SLOVENE/usr/lib/nls/slovene
	! chmog 555 bin bin ../../../../Filesets.sl/SLOVENE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SLOVENE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SLOVENE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SLOVENE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset SPANISH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/iso88591
	! cp spanish.iso88591 ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/iso88591/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/iso88591/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset SPANISH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish
	! cp spanish ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/spanish
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SPANISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset SWEDISH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/iso88591
	! cp swedish.iso88591 ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/iso88591/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/iso88591/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/iso88591
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset SWEDISH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish
	! cp swedish ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/swedish
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SWEDISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset THAI
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/THAI/usr/lib/nls/thai
	! cp thai ../../../../Filesets.sl/THAI/usr/lib/nls/thai/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/THAI/usr/lib/nls/thai/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/THAI/usr/lib/nls/thai
	! chmog 555 bin bin ../../../../Filesets.sl/THAI/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/THAI/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/THAI/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/THAI/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset TURKISH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/iso88599
	! cp turkish.iso88599 ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/iso88599/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/iso88599/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/iso88599
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/nlsinf to the . relative fileset TURKISH
# change mode, owner, and group on /usr/lib/nlsinf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish
	! cp turkish ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/locale.inf
	! chmog 0444 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish/locale.inf
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/turkish
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/TURKISH/
	! chmog 555 bin bin ../../../../Filesets.sl/
