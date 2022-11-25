# @(#) $Revision: 66.2 $

#
# Makefile for find with rcs stuff in it
#
CC	= cc
CFLAGS	= -DNLS -DNLS8 -DNLS16
FLAGS   = -DRCSTOYS
LDFLAGS	= -s
ROOT	=
BIN	= $(ROOT)/bin
CATLOC  = $(ROOT)/usr/lib/nls/n-computer
FINDMSG = /usr/bin/findmsg
GENCAT  = /usr/bin/gencat
OBJECTS = find.o walkfs.o rcstoys.o hpux_rel.o

find : $(OBJECTS)
	$(CC) $(LDFLAGS) -o find $(OBJECTS)

find.cat: find.c
	$(FINDMSG) find.c > find.msg
	$(GENCAT)  $@ find.msg
	/bin/rm  find.msg 
		
release: find find.cat
	cpset find $(BIN)/find 555 bin bin
	cpset find.cat $(CATLOC)/find.cat 444 bin bin

rcsfind: $(OBJECTS:.o=.c) rcstoys.c rcstoys.h
	$(MAKE) -f find.mk CFLAGS="$(CFLAGS) -DRCSTOYS" \
	    rcstoys.o $(OBJECTS)
	$(CC) $(LDFLAGS) -o rcsfind $(OBJECTS) rcstoys.o

clean:
	rm -f *.o find

hpux_rel.o: hpux_rel.c
	sed -e 's/ \$$"/ (with rcs additions) $$"/' <hpux_rel.c >tmp.c
	$(CC) -c $(CFLAGS) $(FLAGS) tmp.c -o hpux_rel.o
	rm -f tmp.c

.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<

find.o: rcstoys.h walkfs.h
rcstoys.o: rcstoys.h walkfs.h
walkfs.o: walkfs.h
