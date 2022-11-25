# @(#) $Revision: 72.1 $
#
# S300 makefile for ccom et al
#

INC    = /usr/include
LINC   = /usr/contrib/include
XCFLAGS= -O -I$(LINC)  -W c,-Yu $(PROCFLAGS) -UDEBUGGING -Ucookies -DFORTY -DDBL8 -DINST_SCHED

LIBS   = -lmalloc
COMMAND= ccom
D      = .

#
# EXPLANATION OF XCFLAGS:
#
#    mc68000	  surrounds machine dependent code (defined in macdefs)
#    R2REGS	  enables "double indexed" addressing modes
#    DOUBLESONLY  if defined, all floats are translated into doubles
#		  within the ccom scanner.
#    DEBUGGING    if defined, debugging flags and functions are
#		  compiled.

OBJ1 = \
    $(D)/cdbsyms.o	\
    $(D)/code.o		\
    $(D)/comm1.o	\
    $(D)/local.o	\
    $(D)/messages.o	\
    $(D)/optim.o	\
    $(D)/pftn.o		\
    $(D)/sa.o		\
    $(D)/scan.o		\
    $(D)/trees.o	\
    $(D)/vtlib.o	\
    $(D)/xdefs.o

P1OBJ = $(D)/cput.o

OBJ2  = \
    $(D)/allo.o		\
    $(D)/local2.o	\
    $(D)/match.o	\
    $(D)/order.o	\
    $(D)/reader.o	\
    $(D)/table.o

P2OBJ = \
    $(D)/backend.o	\
    $(D)/comm2.o

OBJ3  = \
    $(D)/print.o	\
    $(D)/walkf.o

HEADS =	common commonb macdefs mac2defs mfile1 mfile2 manifest \
        cdbsyms.h messages.h

all:	$(COMMAND)

ccom:	$(D)/hpux_rel.o $(OBJ1) $(OBJ2) $(OBJ3) $(D)/cgram.o
	$(CC) -o ccom $(OBJ1) $(OBJ2) $(OBJ3) \
	    $(D)/cgram.o $(D)/hpux_rel.o $(LIBS)

ccom.ansi: $(D)/hpux_rel.o $(OBJ1) $(OBJ2) $(OBJ3) $(D)/cgram.o
	$(CC) -o ccom.ansi $(OBJ1) $(OBJ2) $(OBJ3) \
	    $(D)/cgram.o $(D)/hpux_rel.o $(LIBS) -lm -lcpp

cpass1:	$(D)/hpux_rel.o $(OBJ1) $(OBJ3) $(P1OBJ) $(D)/cgram.o
	$(CC) -o cpass1 $(OBJ1) $(OBJ3) $(P1OBJ) \
	    $(D)/cgram.o $(D)/hpux_rel.o $(LIBS)

cpass1.ansi: $(D)/hpux_rel.o $(OBJ1) $(OBJ3) $(P1OBJ) $(D)/cgram.o
	$(CC) -o cpass1.ansi $(OBJ1) $(OBJ3) $(P1OBJ) \
	    $(D)/cgram.o $(D)/hpux_rel.o $(LIBS) -lm -lcpp

cpass2:	$(D)/hpux_rel.o $(OBJ2) $(OBJ3) $(P2OBJ)
	$(CC) -o cpass2 $(OBJ2) $(OBJ3) $(P2OBJ) \
	    $(D)/hpux_rel.o $(LIBS)

clean:
	rm -f $(OBJ1) $(OBJ2) $(P1OBJ) $(P2OBJ) \
	    $(D)/gram.y $(D)/cgram.c $(D)/cgram.o \
	    $(D)/print.o $(D)/walkf.o $(D)/hpux_rel.o

clobber: clean
	rm -f $(COMMAND)

#
# Dependencies and rules
#

mfile1: macdefs manifest

mfile2: macdefs mac2defs manifest

$(D)/hpux_rel.o: hpux_rel.c
	$(CC) -c $(XCFLAGS) hpux_rel.c -o $@

#
# Rules for making cgram.o from cgram.y
#
$(D)/gram.y:	cgram.y
	-unifdef $(ANSIFLAGS) -UIRIF -ULINT -UAPEX -Uxcomp300_800 cgram.y > $@

$(D)/cgram.c: $(D)/gram.y
	cd $(D); yacc -Xp../yaccpar gram.y
	mv $(D)/y.tab.c $(D)/cgram.c

$(D)/cgram.o: $(D)/cgram.c
	$(CC) -c $(XCFLAGS) +O1 -I. $(D)/cgram.c -o $@

#
# OBJ1 rules  (all depend on mfile1 and messages.h)
#
$(D)/cdbsyms.o: mfile1 messages.h $(LINC)/symtab.h $(LINC)/dnttsizes.h \
		cdbsyms.h cdbsyms.c
	$(CC) -c $(XCFLAGS) cdbsyms.c -o $@

$(D)/code.o: mfile1 messages.h code.c
	$(CC) -c $(XCFLAGS) code.c -o $@

$(D)/comm1.o: mfile1 messages.h common commonb comm1.c
	$(CC) -c $(XCFLAGS) comm1.c -o $@

$(D)/local.o: mfile1 messages.h local.c
	$(CC) -c $(XCFLAGS) local.c -o $@

$(D)/messages.o: mfile1 messages.h messages.c
	$(CC) -c $(XCFLAGS) messages.c -o $@

$(D)/optim.o: mfile1 messages.h optim.c
	$(CC) -c $(XCFLAGS) optim.c -o $@

$(D)/pftn.o: mfile1 messages.h pftn.c
	$(CC) -c $(XCFLAGS) pftn.c -o $@

$(D)/sa.o: mfile1 messages.h $(LINC)/symtab.h sa.c
	$(CC) -c $(XCFLAGS) sa.c -o $@

$(D)/scan.o: mfile1 messages.h scan.c
	$(CC) -c $(XCFLAGS) scan.c -o $@

$(D)/trees.o: mfile1 messages.h trees.c
	$(CC) -c $(XCFLAGS) trees.c -o $@

$(D)/vtlib.o: mfile1 messages.h vtlib.c
	$(CC) -c $(XCFLAGS) vtlib.c -o $@

$(D)/xdefs.o: mfile1 messages.h xdefs.c
	$(CC) -c $(XCFLAGS) xdefs.c -o $@

#
# P1OBJ1 rules (all depend on mfile1 and manifest)
#
$(D)/cput.o: mfile1 manifest cput.c
	$(CC) -c $(XCFLAGS) cput.c -o $@

#
# OBJ2 rules (all depend on mfile2)
#
$(D)/allo.o: mfile2 allo.c
	$(CC) -c $(XCFLAGS) allo.c -o $@

$(D)/local2.o: mfile2 local2.c
	$(CC) -c $(XCFLAGS) local2.c -o $@

$(D)/match.o: mfile2 match.c
	$(CC) -c $(XCFLAGS) match.c -o $@

$(D)/order.o: mfile2 order.c
	$(CC) -c $(XCFLAGS) order.c -o $@

$(D)/reader.o: mfile2 reader.c
	$(CC) -c $(XCFLAGS) reader.c -o $@

$(D)/table.o: mfile2 table.c
	$(CC) -c $(XCFLAGS) table.c -o $@

#
# P2OBJ2 rules (all depend on mfile2 and manifest)
#
$(D)/backend.o: mfile2 manifest backend.c
	$(CC) -c $(XCFLAGS) backend.c -o $@

$(D)/comm2.o: manifest mfile2 common commonb comm2.c
	$(CC) -c $(XCFLAGS) comm2.c -o $@

#
# OBJ3 rules (assembly source)
#
$(D)/print.o: print.s
	as print.s -o $@

$(D)/walkf.o: walkf.s
	as walkf.s -o $@
