# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/vis/makefile,v $
# $Revision: 64.2.1.6 $

# Common makefile for vis

FINDMSG = /usr/bin/findmsg
GENCAT = /usr/bin/gencat

default: all

all:	vis vis.cat

vis:	vis.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o vis vis.c

clean:
	/bin/rm -f vis.o vis.msg

clobber: clean
	 /bin/rm -f vis vis.cat

vis.cat: vis.msg

#
# We define two rules for creating .cat and .msg files so make can do that
# work for us.  With these rules, it is only necessary to specify that the
# .cat depends on the .msg.  Make will automatically know to create the .msg
# from the .c and how to create the .cat file.
#

.c.msg:
	${FINDMSG} $*.c >$*.msg

.msg.cat:
	${GENCAT} $*.cat $*.msg

.SUFFIXES: .cat .msg

# copy /usr/bin/vis to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/vis
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	vis
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp vis ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/vis
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/vis to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/vis
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp vis ../../../../Filesets.sl/CMDS-AUX/usr/bin/vis
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/vis
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/vis to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/vis
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp vis.cat ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/vis.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/vis.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/vis to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/vis
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! ln ../../../../Filesets.sl/CMDS-AUX/usr/bin/vis ../../../../Filesets.sl/CMDS-AUX/usr/bin/inv
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
