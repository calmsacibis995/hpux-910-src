# @(#) $Revision: 70.1.1.5 $

#----------------------------------------------------------------------
#
# getconf makefile.
#
# Jer/ Eberhard,
# Hewlett-Packard UDL - HP-UX COMMANDS LAB, Fort Collins, CO
#----------------------------------------------------------------------

CFLAGS = -DNLS -DPOSIX

SRCS = getconf.c

OBJS = $(SRCS:.c=.o)

HDRS = /usr/include/unistd.h getconf.h

default: getconf getconf.cat

getconf: $(HDRS) $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

getconf.cat: getconf.c
	findmsg getconf.c | sort -n >getconf.msg
	gencat getconf.cat getconf.msg

getconf.h:
	chmod 555 awk.sh; ./awk.sh

clean:
	/bin/rm -f $(OBJS) getconf.msg

clobber: clean
	/bin/rm -f a.out getconf getconf.cat

# copy /usr/bin/getconf to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/getconf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	getconf
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp getconf ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/getconf
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/getconf to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/getconf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp getconf ../../../../Filesets.sl/CMDS-AUX/usr/bin/getconf
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/getconf
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/getconf to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/getconf
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp getconf.cat ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/getconf.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/getconf.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
