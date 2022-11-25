# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/spell/makefile,v $
# $Revision: 66.5.1.12 $

# Common makefile for spell utility

SFILES = spellprog.c spellin.c
DFILES = american british local list extra stop

PROGS  = spell hlista hlistb hstop spellin spellprog hashmake \
	 hashcheck compress

default all:    $(PROGS)

spell:	spellprog spell.sh
	cp spell.sh spell

compress:  compress.sh
	cp compress.sh compress

spellprog: spellprog.o hash.o hashlook.o huff.o comm.o
	$(CC) $(LDFLAGS) spellprog.o hash.o hashlook.o \
		huff.o comm.o -o spellprog

spellin: spellin.o huff.o
	$(CC) $(LDFLAGS) spellin.o huff.o -o spellin

hashcheck: hashcheck.o hash.o huff.o
	$(CC) $(LDFLAGS) hashcheck.o hash.o huff.o -o hashcheck 

hashmake: hashmake.o hash.o
	$(CC) $(LDFLAGS) hashmake.o hash.o -o hashmake

alldata: hlista hlistb hstop
	/bin/rm -f htemp1

htemp1:	list local extra hashmk1
	cat list local extra | ./hashmk1 >htemp1

hlista: american hashmake hashmk1 spellin spellin1 htemp1
	./hashmk1 <american |sort -u - htemp1 >htemp2
	./spellin1 `wc htemp2|sed -n 's/\([^ ]\) .*/\1/p' ` <htemp2 >hlista
	/bin/rm -f htemp2

hlistb: british hashmk1 spellin1 htemp1
	./hashmk1 <british |sort -u - htemp1 >htemp2
	./spellin1 `wc htemp2|sed -n 's/\([^ ]\) .*/\1/p' ` <htemp2 >hlistb 
	/bin/rm -f  htemp2

hstop:	stop spellin1 hashmk1
	./hashmk1 <stop | sort -u >htemp2
	./spellin1 `wc htemp2|sed -n 's/\([^ ]\) .*/\1/p' ` <htemp2 >hstop
	/bin/rm -f  htemp2

# used to be compiled with cc.unshared
hashmk1: hashmake.c hash.c
	$(CC) -Wl,-a,archive $(LDFLAGS) hashmake.c hash.c -o hashmk1

# used to be compiled with cc.unshared
spellin1: spellin.c huff.c
	$(CC) -Wl,-a,archive $(LDFLAGS) spellin.c huff.c -o spellin1

clean:
	/bin/rm -f *.o

clobber: clean 
	/bin/rm -f spell spellprog spellin hashmake hlist* hstop spellin1 
	/bin/rm -f hashmk1 htemp1 htemp2 hashcheck compress

# copy /usr/bin/spell to the . relative fileset SPELL
# change mode, owner, and group on /usr/bin/spell
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	spell
	mkdir -p ../../../../Filesets.sl/SPELL/usr/bin
	! cp spell ../../../../Filesets.sl/SPELL/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/SPELL/usr/bin/spell
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/
# copy /usr/bin/spell to the . relative fileset SPELL
# change mode, owner, and group on /usr/bin/spell
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SPELL/etc/newconfig
	! cp hlista ../../../../Filesets.sl/SPELL/etc/newconfig/hlista
	! chmog 0444 bin bin ../../../../Filesets.sl/SPELL/etc/newconfig/hlista
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/etc/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/etc/
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/spell to the . relative fileset SPELL
# change mode, owner, and group on /usr/bin/spell
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SPELL/etc/newconfig
	! cp hlistb ../../../../Filesets.sl/SPELL/etc/newconfig/hlistb
	! chmog 0444 bin bin ../../../../Filesets.sl/SPELL/etc/newconfig/hlistb
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/etc/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/etc/
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/spell to the . relative fileset SPELL
# change mode, owner, and group on /usr/bin/spell
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SPELL/etc/newconfig
	! cp hstop ../../../../Filesets.sl/SPELL/etc/newconfig/hstop
	! chmog 0444 bin bin ../../../../Filesets.sl/SPELL/etc/newconfig/hstop
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/etc/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/etc/
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/spell to the . relative fileset SPELL
# change mode, owner, and group on /usr/bin/spell
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SPELL/usr/bin
	! cp spell ../../../../Filesets.sl/SPELL/usr/bin/spell
	! chmog 0555 bin bin ../../../../Filesets.sl/SPELL/usr/bin/spell
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/spell to the . relative fileset SPELL
# change mode, owner, and group on /usr/bin/spell
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SPELL/usr/lib/spell
	! cp compress ../../../../Filesets.sl/SPELL/usr/lib/spell/compress
	! chmog 0444 bin bin ../../../../Filesets.sl/SPELL/usr/lib/spell/compress
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/lib/spell
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/spell to the . relative fileset SPELL
# change mode, owner, and group on /usr/bin/spell
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SPELL/usr/lib/spell
	! cp hashcheck ../../../../Filesets.sl/SPELL/usr/lib/spell/hashcheck
	! chmog 0555 bin bin ../../../../Filesets.sl/SPELL/usr/lib/spell/hashcheck
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/lib/spell
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/spell to the . relative fileset SPELL
# change mode, owner, and group on /usr/bin/spell
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SPELL/usr/lib/spell
	! cp hashmake ../../../../Filesets.sl/SPELL/usr/lib/spell/hashmake
	! chmog 0555 bin bin ../../../../Filesets.sl/SPELL/usr/lib/spell/hashmake
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/lib/spell
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/spell to the . relative fileset SPELL
# change mode, owner, and group on /usr/bin/spell
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SPELL/usr/lib/spell
	! cp spellin ../../../../Filesets.sl/SPELL/usr/lib/spell/spellin
	! chmog 0555 bin bin ../../../../Filesets.sl/SPELL/usr/lib/spell/spellin
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/lib/spell
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/spell to the . relative fileset SPELL
# change mode, owner, and group on /usr/bin/spell
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SPELL/usr/lib/spell
	! cp spellprog ../../../../Filesets.sl/SPELL/usr/lib/spell/spellprog
	! chmog 0555 bin bin ../../../../Filesets.sl/SPELL/usr/lib/spell/spellprog
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/lib/spell
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SPELL/
	! chmog 555 bin bin ../../../../Filesets.sl/
