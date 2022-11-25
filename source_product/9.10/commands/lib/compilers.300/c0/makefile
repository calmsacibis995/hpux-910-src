# @(#) $Revision: 66.5 $

DEFINES= -DNEWC0SUPPORT -DR2REGS -DFORT -DC0 -DC1SUPPORT -DC1 -DLDBL

#The shared/archive flag
#ARCH=	-Wl,-a,archive

default: c0

c0:	K_rel_c0.o main.o pass1.o callgraph.o pass2.o inline.o
	$(CC) $(ARCH) $(CFLAGS) -o c0 K_rel_c0.o main.o pass1.o callgraph.o pass2.o inline.o -lmalloc
	echo "c0 COMPLETE"

c0.h:		opcodes.h macdefs mac2defs manifest mfile2

main.o:		c0.h commonb messages.h
	$(CC) -c $(DEFINES) $(CFLAGS) main.c

pass1.o:	c0.h
	$(CC) -c $(DEFINES) $(CFLAGS) pass1.c

callgraph.o:	c0.h
	$(CC) -c $(DEFINES) $(CFLAGS) callgraph.c

pass2.o:	c0.h
	$(CC) -c $(DEFINES) $(CFLAGS) pass2.c

inline.o:	c0.h
	$(CC) -c $(DEFINES) $(CFLAGS) inline.c

clean:
	rm -f *.o
clobber:
	rm -f c0 *.o

opcodes.h:	../opcodes.h
	cp ../opcodes.h .
macdefs:	../macdefs
	cp ../macdefs .
mac2defs:	../mac2defs
	cp ../mac2defs .
manifest:	../manifest
	cp ../manifest .
mfile2:		../mfile2
	cp ../mfile2 .
messages.h:	../messages.h
	cp ../messages.h .
commonb:	../commonb
	cp ../commonb .

install:
	:
