# $Source: /misc/source_product/9.10/commands.rcs/lib/compilers.300/c1/makefile,v $
# $Revision: 66.11.1.1 $

# S300 makefile for compilers/c1

FINCFLAGS=	-UDEBUGGING -O -DFORT -Dfast_malloc -UCOUNTING -DNEW_SETS -Drel8_0
M4=		m4
M4FLAGS=	-UDEBUG2
LOADFLAGS=	-lmalloc -s
INCFLAGS=		-I../

#	DEBUGGING	if defined, debugging flags and functions are compiled.

FP2OBJ= c1.o misc.o dag.o oglobal.o sets.o p2out.o loops.o \
	register.o regpass1.o regdefuse.o regweb.o regallo.o \
	regpass2.o symtab.o duopts.o vfe.o loopxforms.o utils.o

default: c1 

c1:	K_rel_c1.o
	$(CC) -o c1 K_rel_c1.o $(FP2OBJ) $(LOADFLAGS)

clean:
	/bin/rm -f $(FP2OBJ)

clobber: clean
	/bin/rm -f c1

# the following describes the ccom compiler.

mfile2: macdefs manifest
	touch mfile2

manifest: ../opcodes.h
	touch manifest

K_rel_c1.o: K_rel_c1.c $(FP2OBJ)
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c K_rel_c1.c

c1.o: mfile2 c1.h c1.c
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c c1.c

utils.o: mfile2 c1.h utils.c
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c utils.c

dag.o: mfile2 c1.h dag.c
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c dag.c

misc.o: mfile2 c1.h misc.c
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c misc.c

oglobal.o: mfile2 c1.h oglobal.c
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c oglobal.c

sets.o: sets.s
	$(M4) $(M4FLAGS) sets.s | /bin/as20 -o sets.o

# The CTAGS and cross refrence entries below must
# also be change (if you are to go back to sets.s)
#sets.o:  sets.c
#	$(CC) $(FINCFLAGS) $(INCFLAGS) -c sets.c

p2out.o: mfile2 c1.h p2out.c
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c p2out.c

loops.o: mfile2 c1.h loops.c
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c loops.c

register.o: mfile2 c1.h
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c register.c

regpass1.o: mfile2 c1.h
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c regpass1.c

regdefuse.o: mfile2 c1.h
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c regdefuse.c

regweb.o: mfile2 c1.h
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c regweb.c

regallo.o: mfile2 c1.h
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c regallo.c

regpass2.o: mfile2 c1.h
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c regpass2.c

symtab.o: mfile2 c1.h
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c symtab.c

duopts.o: mfile2 c1.h
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c duopts.c

vfe.o: mfile2 c1.h
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c vfe.c

loopxforms.o: mfile2 c1.h
	$(CC) $(FINCFLAGS) $(INCFLAGS) -c loopxforms.c

install:
	:
