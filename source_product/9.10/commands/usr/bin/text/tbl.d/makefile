# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/text/tbl.d/makefile,v $
# $Revision: 70.1 $

# Common makefile for tbl

FINDMSG	= /usr/bin/findmsg
GENCAT	= /usr/bin/gencat

OFILES  = t0.o t1.o t2.o t3.o t4.o t5.o t6.o t7.o t8.o t9.o tb.o tc.o\
 	  te.o tf.o tg.o ti.o tm.o tr.o ts.o tt.o tu.o tv.o hpux_rel.o
MSGSRC  = t1.c t3.c t4.c t5.c t6.c t7.c t8.c t9.c tb.c tc.c te.c tg.c tr.c

default: all

all:	tbl tbl.cat

tbl:	$(OFILES) 
	$(CC) $(LDFLAGS) -o tbl $(OFILES)

tbl.cat: $(MSGSRC)
	for i in $(MSGSRC); do \
		$(FINDMSG) $$i >tbl.msg;\
		$(GENCAT) $@ tbl.msg;\
	done

$(OFILES): t..c

clean:
	/bin/rm -f *.o tbl.msg

clobber: clean
	/bin/rm -f tbl tbl.cat
