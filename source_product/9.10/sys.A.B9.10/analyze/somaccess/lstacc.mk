#   Makefile for somaccess rountines running on SPECTRUM
# 
#   AUTHOR
#	J. Hofmann	June 26, 1985
# 
#   SYNOPSIS
#
#       This makefile is for the somaccess routines,
#       which are used by xdb, pxdb, and dsymtab.


# Directory abbreviations

DEVEL   = /mnt/hpe/sysint/jjh/xdb/devel

#CC 	= $(DEVEL)/tools/cc/cc
I1	= $(DEVEL)/sominclude 
SRC     = $(DEVEL)/hpesrc

DEFS	= -DSPECTRUM -DINTERIMCALL

CFLAGS 	= $(DEFS) -O1:r:f -I$(I1)


.s.o:
	$(ASM) $*.s

.c.o:
	$(CC) $(CFLAGS) -c -g $*.c
	cp $*.c $(SRC)/$*.out
        

OBJS	= lstacc.o


cconly:	$(OBJS)


clean:
	rm -f $(OBJS) core


clobber: clean
