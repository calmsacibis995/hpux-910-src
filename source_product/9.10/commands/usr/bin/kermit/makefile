# @(#) $Revision: 66.2.1.4 $ 
# CKUKER.MAK, Version 2.13, 24 January 1989
#
# -- Makefile to build C-Kermit for Unix and Unix-like systems --
#
#
###########################################################################
#
# Dependencies Section:
#

OBJS =  ckcmai.o hpux_rel.o ckucmd.o ckuusr.o ckuus2.o ckuus3.o \
	ckcpro.o ckcfns.o \
	 ckcfn2.o ckucon.o ckutio.o ckufio.o ckudia.o ckuscr.o

FLAGS = -DUXIII -DHPUX -DDEBUG -DTLOG 

default: kermit

kermit: $(OBJS)
	$(CC) $(CFLAGS) $(FLAGS) -o kermit $(OBJS)

#	$(CC) $(CFLAGS) $(FLAGS) -o kermit ckcmai.o ckutio.o ckufio.o ckcfns.o \
#		 ckcfn2.o ckcpro.o ckucmd.o ckuus2.o ckuus3.o ckuusr.o \
#		 ckucon.o ckudia.o ckuscr.o $(LIBS)

#
# Re-define .c.o rule so implicit complies will use FLAGS
#


.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<

ckcmai.o: ckcmai.c ckcker.h ckcdeb.h ckcsym.h

ckuusr.o: ckuusr.c ckucmd.h ckcker.h ckuusr.h ckcdeb.h

ckuus2.o: ckuus2.c ckucmd.h ckcker.h ckuusr.h ckcdeb.h

ckuus3.o: ckuus3.c ckucmd.h ckcker.h ckuusr.h ckcdeb.h

ckucmd.o: ckucmd.c ckucmd.h ckcdeb.h

ckcpro.o: ckcpro.c ckcker.h ckcdeb.h

ckcpro.c: ckcpro.w wart
	./wart ckcpro.w ckcpro.c

ckcfns.o: ckcfns.c ckcker.h ckcdeb.h ckcsym.h

ckcfn2.o: ckcfn2.c ckcker.h ckcdeb.h ckcsym.h

ckufio.o: ckufio.c ckcker.h ckcdeb.h

ckutio.o: ckutio.c ckcdeb.h

ckucon.o: ckucon.c ckcker.h ckcdeb.h

wart: ckwart.o
	$(CC) -Wl,-a,archive $(CFLAGS) $(FLAGS) -o wart ckwart.o

ckwart.o: ckwart.c

ckudia.o: ckudia.c ckcker.h ckcdeb.h ckucmd.h

ckuscr.o: ckuscr.c ckcker.h ckcdeb.h



clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f kermit wart ckcpro.c 
#
#HP 9000 series 300, 500, 800.
#hpux:
#	make wermit "CFLAGS = -DUXIII -DHPUX -DDEBUG -DTLOG -O" "LNKFLAGS ="
#

# copy /usr/bin/kermit to the . relative fileset SYSCOM
# change mode, owner, and group on /usr/bin/kermit
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	kermit
	mkdir -p ../../../../Filesets.sl/SYSCOM/usr/bin
	! cp kermit ../../../../Filesets.sl/SYSCOM/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/SYSCOM/usr/bin/kermit
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/
# copy /usr/bin/kermit to the . relative fileset SYSCOM
# change mode, owner, and group on /usr/bin/kermit
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYSCOM/usr/bin
	! cp kermit ../../../../Filesets.sl/SYSCOM/usr/bin/kermit
	! chmog 0555 bin bin ../../../../Filesets.sl/SYSCOM/usr/bin/kermit
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/
	! chmog 555 bin bin ../../../../Filesets.sl/
