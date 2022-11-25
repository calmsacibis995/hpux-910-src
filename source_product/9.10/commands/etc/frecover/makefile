# $Source: /misc/source_product/9.10/commands.rcs/etc/frecover/makefile,v $
# $Revision: 72.6 $

# Common makefile for frecover

# CFLAGS= -DDEBUG -DDEBUG_T -DDEBUG_D
LIBS=-lmalloc

GENCAT= /usr/bin/gencat
FINDMSG= /usr/bin/findmsg -a

FILES=	    main.c utilities.c files.c io.c volheaders.c hpux_rel.c rmt.c vdi.c
OBJS=	    main.o utilities.o files.o io.o volheaders.o hpux_rel.o rmt.o vdi.o
MSGS= main.msg utilities.msg files.msg io.msg volheaders.msg

default: frecover frecover.cat

metrics: frecover.M

frecover: $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o frecover $(OBJS) $(LIBS)

frecover.M: $(FILES)
	$(METRICS) $(CFLAGS) -o frecover.M $(FILES) $(LIBS)

clean:
	/bin/rm -f *.o $(MSGS)

clobber: clean
	/bin/rm -f frecover frecover.cat

frecover.cat: $(MSGS)
	$(GENCAT) $@ $(MSGS)

#
# We define two new rules, for creaing .msg and .cat files.  With these rules,
# make knows how to automatically create .msg files from .c files, and .cat
# files from .msg files.  This allows us to merely specify that the .cat
# depends on the .msg; make will create the .msg file for us if necessary.
#

.c.msg:
	$(FINDMSG) $*.c > $*.msg

.msg.cat:
	$(GENCAT) $*.cat $*.msg

.SUFFIXES: .cat .msg

$(OBJS): frecover.h 

# copy /etc/frecover to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/frecover
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	frecover
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp frecover ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/frecover
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/frecover to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/frecover
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp frecover ../../../Filesets.sl/SYS-ADMIN/etc/frecover
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/frecover
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/frecover to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/frecover
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/usr/lib/nls/C
	! cp frecover.cat ../../../Filesets.sl/SYS-ADMIN/usr/lib/nls/C/frecover.cat
	! chmog 0444 bin bin ../../../Filesets.sl/SYS-ADMIN/usr/lib/nls/C/frecover.cat
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
