#!/bin/make -f
# pathalias -- by steve bellovin, as told to peter honeyman
# /* @(#) $Revision: 66.3.1.1 $ */

### begin configuration section
###
# if you can't or don't intend to use dbm files,
# don't bother with DBM or makedb
DBM = -ldbm
# or if you roll your own ...
# DBM = dbm.o
###
# where is getopt (if not in the c library)?
# GETOPT = getopt.o
### end of configuration section 


CC = cc
CFLAGS = -O
LDFLAGS = -s $(GETOPT)
YFLAGS = -d

OBJ = addlink.o addnode.o local.o main.o mapit.o mapaux.o mem.o parse.o printit.o
HDRS = def.h config.h
CSRC = addlink.c addnode.c local.c main.c mapit.c mapaux.c mem.c printit.c
LSRC = $(CSRC) parse.c
SRC = $(CSRC) parse.y makedb.c arpatxt.c

default: pathalias

pathalias: $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o pathalias

all: pathalias makedb arpatxt

$(OBJ):	$(HDRS)

parse.c: parse.y $(HDRS)
	$(YACC) $(YFLAGS) parse.y
	mv y.tab.c parse.c

makedb: makedb.o
	$(CC) makedb.o $(LDFLAGS) $(DBM) -o makedb

makedb.o: config.h

arpatxt: arpatxt.o
	$(CC) arpatxt.o $(LDFLAGS) -o arpatxt

clean:
	rm -f *.o y.tab.? parse.c

clobber: clean
	rm -f pathalias makedb arpatxt

tags: $(SRC) $(HDRS)
	ctags -w $(HDRS) $(SRC)

bundle:
	@bundle README CHANGES pathalias.1 Makefile ${HDRS} ${SRC}

lint:	$(LSRC)
	lint $(CFLAGS) $(LSRC)
	lint makedb.c
	lint arpatxt.c

install:
	@echo "install pathalias, makedb, arpatxt, and pathalias.1"
	@echo "according to local conventions"
# copy /usr/bin/pathalias to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/pathalias
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/bin
	! cp pathalias ../../../../Filesets.sl/MAILERS/usr/bin/pathalias
	! chmog 0555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin/pathalias
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
