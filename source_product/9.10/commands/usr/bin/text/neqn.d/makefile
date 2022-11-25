# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/text/neqn.d/makefile,v $
# $Revision: 70.1 $

# Common makefile for neqn (text subsystem)

PFLAGS  = -DNEQN
YFLAGS  = -d
FINDMSG	= /usr/bin/findmsg
GENCAT	= /usr/bin/gencat
LIBS	= -ly
OFILES  = diacrit.o eqnbox.o font.o fromto.o funny.o glob.o integral.o \
	  io.o lex.o lookup.o mark.o matrix.o move.o over.o paren.o \
	  pile.o shift.o size.o sqrt.o text.o hpux_rel.o
MSGSRC	= funny.c io.c lex.c lookup.c sqrt.c text.c

default: all

all:    neqn neqn.cat

neqn:   e.o $(OFILES)
	$(CC) $(LDFLAGS) -o neqn $(OFILES) e.o $(LIBS)

$(OFILES): e.h e.def

e.def:    y.tab.h
	  -cmp -s y.tab.h e.def || cp y.tab.h e.def

y.tab.h:  e.o

neqn.cat: $(MSGSRC)
	for i in $(MSGSRC); do \
		$(FINDMSG) $$i >neqn.msg;\
		$(GENCAT) $@ neqn.msg;\
	done

clean:
	/bin/rm -f *.o y.tab.[ch] e.def neqn.msg

clobber:  clean
	/bin/rm -f neqn neqn.cat

# Redefine the .c to .o rule as to include PFLAGS
.c.o:
	$(CC) $(CFLAGS) $(PFLAGS) -c $<
