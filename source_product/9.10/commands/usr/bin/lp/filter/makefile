# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/lp/filter/makefile,v $
# $Revision: 72.1 $

# Common makefile for lp/filter files:
# lprpp, reverse
# divpage, fontdl, plotdvr, plotstat, printstat, rwrite

DFLAGS	= -DREMOTE
FLAGS = -lm
HFLAGS = -ldvio
FILES   = lprpp.o reverse.o rwrite.o \
	divpage.o fontdl.o plotdvr.o plotstat.o printstat.o

default: all

all: lprpp reverse rwrite \
	divpage fontdl printstat plotdvr 

metrics: lprpp.M reverse.M rwrite.M \
	divpage.M fontdl.M printstat.M plotdvr.M

lprpp: lprpp.o
	$(CC) $(CFLAGS) lprpp.o $(LDFLAGS) -o lprpp

lprpp.M: lprpp.c
	$(METRICS) $(CFLAGS) lprpp.c $(LDFLAGS) -o lprpp.M

reverse: reverse.o
	$(CC) $(CFLAGS) reverse.o $(LDFLAGS) -o reverse

reverse.M: reverse.c
	$(METRICS) $(CFLAGS) reverse.c $(LDFLAGS) -o reverse.M

rwrite:	rwrite.o
	$(CC) $(CFLAGS) rwrite.o $(LDFLAGS) -o rwrite

rwrite.M:	rwrite.c
	$(METRICS) $(CFLAGS) rwrite.c $(LDFLAGS) -o rwrite.M

divpage:	divpage.o
	$(CC) $(CFLAGS) divpage.o $(LDFLAGS) -o divpage

divpage.M:	divpage.c
	$(METRICS) $(CFLAGS) divpage.c $(LDFLAGS) -o divpage.M

fontdl:	fontdl.o
	$(CC) $(CFLAGS) fontdl.o $(LDFLAGS) -o fontdl

fontdl.M:	fontdl.c
	$(METRICS) $(CFLAGS) fontdl.c $(LDFLAGS) -o fontdl.M

plotdvr: plotdvr.o plotstat.o
	$(CC) $(CFLAGS) plotdvr.o plotstat.o $(HFLAGS) $(FLAGS) $(LDFLAGS) -o plotdvr

plotdvr.M: plotdvr.c plotstat.c
	$(METRICS) $(CFLAGS) plotdvr.c plotstat.c $(HFLAGS) $(FLAGS) $(LDFLAGS) -o plotdvr.M

printstat:	printstat.o
	$(CC) $(CFLAGS) printstat.o $(LDFLAGS) -o printstat

printstat.M:	printstat.c
	$(METRICS) $(CFLAGS) printstat.c $(LDFLAGS) -o printstat.M

plotstat.o:
	$(CC) $(CFLAGS) $(DFLAGS) -c plotdvr.c $<

rwrite.o:
	$(CC) $(CFLAGS) $(DFLAGS) -c $<

clean:
	/bin/rm -f $(FILES)

clobber:  clean
	/bin/rm -f lprpp reverse rwrite \
	divpage fontdl plotdvr printstat \
	lprpp.M reverse.M rwrite.M \
	divpage.M fontdl.M plotdvr.M printstat.M
