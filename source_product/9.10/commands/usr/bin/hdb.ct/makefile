# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/hdb.ct/makefile,v $
# $Revision: 64.2.1.2 $

# Common makefile for ct

FINDMSG = /usr/bin/findmsg
GENCAT = /usr/bin/gencat

# This assumes that the uucp directory is also in shared source (i.e. on
# the same level in the source tree
UUDIR = ../hdb.uucp

FLAGS = -DSTANDALONE

CTOBJS =  ct.o callers.o getargs.o line.o uucpdefs.o ulockf.o\
	 conn.o hpux_rel.o

CTMSGS = ct.msg $(UUDIR)/callers.msg $(UUDIR)/getargs.msg \
	$(UUDIR)/line.msg $(UUDIR)/uucpdefs.msg $(UUDIR)/ulockf.msg \
	 $(UUDIR)/conn.msg

default: ct ct.cat

ct:  $(CTOBJS)
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) $(CTOBJS) -o ct

clean:
	/bin/rm -f *.o *.msg

clobber: clean
	/bin/rm -f ct ct.cat

callers.o:	$(UUDIR)/callers.c   $(UUDIR)/uucp.h $(UUDIR)/parms.h
	$(CC) -c $(CFLAGS) $(FLAGS) $(UUDIR)/callers.c

getargs.o:	$(UUDIR)/getargs.c   $(UUDIR)/uucp.h $(UUDIR)/parms.h
	$(CC) -c $(CFLAGS) $(FLAGS) $(UUDIR)/getargs.c
 
line.o:		$(UUDIR)/line.c   $(UUDIR)/uucp.h $(UUDIR)/parms.h
	$(CC) -c $(CFLAGS) $(FLAGS) $(UUDIR)/line.c

uucpdefs.o:	$(UUDIR)/uucpdefs.c  $(UUDIR)/uucp.h $(UUDIR)/parms.h
	$(CC) -c $(CFLAGS) $(FLAGS) $(UUDIR)/uucpdefs.c

ulockf.o:	$(UUDIR)/ulockf.c  $(UUDIR)/uucp.h $(UUDIR)/parms.h
	$(CC) -c $(CFLAGS) $(FLAGS) $(UUDIR)/ulockf.c

conn.o:		$(UUDIR)/conn.c
	$(CC) -c $(CFLAGS) $(FLAGS) $(UUDIR)/conn.c

ct.cat: $(CTMSGS)
	$(GENCAT) $@ $(CTMSGS)

#
# We define two new rules for creating .msg and .cat files so that we can
# let make's internal rules do the work for us.  With these rules, we only
# need to specify that the .cat depends on the .msg; make will know it needs
# to create the .msg from the .c automatically.
#
.c.msg:
	$(FINDMSG) $*.c > $*.msg

.msg.cat:
	$(GENCAT) $*.cat $*.msg

.SUFFIXES: .cat .msg

#
# We re-define the .c.o rule so that implicit compiles to .o files will
# use the FLAGS macro defined above.
#
.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<
# copy /usr/bin/hdb.ct to the . relative fileset SYSCOM
# change mode, owner, and group on /usr/bin/hdb.ct
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/SYSCOM/usr/bin
	! cp ct ../../../../Filesets.sl/SYSCOM/usr/bin/ct
	! chmog 04555 root bin ../../../../Filesets.sl/SYSCOM/usr/bin/ct
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/hdb.ct to the . relative fileset SYSCOM
# change mode, owner, and group on /usr/bin/hdb.ct
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYSCOM/usr/lib/nls/C
	! cp ct.cat ../../../../Filesets.sl/SYSCOM/usr/lib/nls/C/ct.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/SYSCOM/usr/lib/nls/C/ct.cat
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/
	! chmog 555 bin bin ../../../../Filesets.sl/
