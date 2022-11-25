# @(#) $Revision: 1.4 $    

ROOT = /source/newfs
INSDIR = $(ROOT)/usr/contrib/bin
INSMAN = $(ROOT)/usr/contrib/man/man1m
CFLAGS=-O -I../Filesets/BUILDENV/usr/include
LDFLAGS = -s

all: convertmts
	:
convertmts: convertmts.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o convertmts convertmts.c

install: release

release: convertmts
	cp    convertmts  $(INSDIR)/convertmts
	chmod 555   $(INSDIR)/convertmts
	chgrp bin   $(INSDIR)/convertmts
	chown bin   $(INSDIR)/convertmts
	cp    convertmts.1m  $(INSMAN)/convertmts.1m
	chmod 444   $(INSMAN)/convertmts.1m
	chgrp bin   $(INSMAN)/convertmts.1m
	chown bin   $(INSMAN)/convertmts.1m

clean:
	:

clobber:  clean
	  rm -f convertmts
