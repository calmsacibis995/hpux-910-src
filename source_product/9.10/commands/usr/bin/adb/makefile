# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/adb/makefile,v $
# $Revision: 66.6 $

# S300 makefile for adb

VERSION =	hpux_rel
FLAGS	=	

# flags for adb with ksh style line editing
# FLAGS	=	-DMANX -DKSH

OBJS =		$(VERSION).o\
		access.o command.o expr.o\
		format.o input.o main.o message.o\
		opset.o output.o pcs.o print.o\
		process.o runpcs.o setup.o sym.o float.o

default:	adb

adb:		$(OBJS)
		$(CC) $(LDFLAGS) $(OBJS) -o adb

adb.ksh:	$(OBJS)
		$(CC) $(LDFLAGS) $(OBJS) le.a -o adb.ksh

$(OBJS):	defs.h

clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f adb

#
# Re-define .c.o rule so implicit compiles will use FLAGS
#

.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<

# copy /usr/bin/adb to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/adb
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	adb
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp adb ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/adb
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/adb to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/adb
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp adb ../../../../Filesets.sl/CMDS-MIN/usr/bin/adb
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/adb
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
