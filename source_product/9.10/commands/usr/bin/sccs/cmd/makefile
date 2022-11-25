# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/sccs/cmd/makefile,v $
# $Revision: 66.1 $

# Common makefile for the cmds' of sccs
# The old makefile used to also make several links,
#	rmdel to rmchg
#	sact  to unget
#	cdc   to rmchg
# these links will be done by the build process.

LIBS = ../lib/comobj.a	\
	../lib/mpwlib.a	\
	../lib/cassi.a  \
	-lPW

# All commands built by makefile
CMDS  = admin comb delta get help prs rmchg unget val vc \
	what sccsdiff help2

LINKS = cdc rmdel sact

all:	$(CMDS) 

admin: 	admin.c
	$(CC) $(LDFLAGS) -o admin admin.c $(LIBS) -lBUILD

comb:	comb.c
	$(CC) $(LDFLAGS) -o comb comb.c $(LIBS) 

delta: 	delta.c
	$(CC) $(FLAGS) $(LDFLAGS) -o delta delta.c $(LIBS) 

get:	get.c
	$(CC) $(LDFLAGS) -o get get.c $(LIBS) 

help:	help.c
	$(CC) $(LDFLAGS) -o help help.c

help2:	help2.c
	$(CC) $(LDFLAGS) -o help2 help2.c $(LIBS)

prs:	prs.c
	$(CC) $(LDFLAGS) -o prs prs.c $(LIBS) 

rmchg:	rmchg.c
	$(CC) $(LDFLAGS) -o rmchg rmchg.c $(LIBS) 

unget:	unget.c
	$(CC) $(LDFLAGS) -o unget unget.c $(LIBS) 

val:	val.c
	$(CC) $(LDFLAGS) -o val val.c $(LIBS)

vc:	vc.c
	$(CC) $(LDFLAGS) -o vc vc.c $(LIBS)

what:	what.c
	$(CC) $(LDFLAGS) -o what what.c $(LIBS)

sccsdiff:	sccsdiff.sh
	cp sccsdiff.sh sccsdiff

clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f $(CMDS) 
	/bin/rm -f $(LINKS)
