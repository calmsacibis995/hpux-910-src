#   Makefile for somaccess rountines running on SPECTRUM
# 
#   AUTHOR
#	J. Hofmann	June 26, 1985
#       S. Lilker       Oct  23, 1986
# 
#   SYNOPSIS
#
#       This makefile is for the somaccess routines,
#       which are used by xdb, pxdb, and dsymtab.


# Directory abbreviations

ROOT    = /mnt/art/joh/src

CC 	= /bin/cc

HPESRC  = $(ROOT)/junk

DEFS	=

CFLAGS 	= $(DEFS)


.s.o:
	$(ASM) $*.s

.c.o:
	$(CC) $(CFLAGS) -c $*.c
	cp $*.c $(HPESRC)/$*.out
        

OBJS	= lstacc.o somacc.o


cconly:	$(OBJS)


clean:
	rm -f $(OBJS) core


clobber: clean
