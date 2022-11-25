# @(#) $Revision: 66.5.1.5 $
#
# makefile for diff 
#

DIFF=	/bin/diff
DIFFH=  /usr/lib/diffh
PR=	/bin/pr
FLAGS=  -DDIFF='"${DIFF}"' -DDIFFH='"${DIFFH}"' -DPR='"${PR}"' 

SRCS=	diff.c diffdir.c diffreg.c hpux_rel.c
HDRS=	diff.h
OBJS=	diff.o diffdir.o diffreg.o hpux_rel.o

FINDMSG = /usr/bin/findmsg
GENCAT = /usr/bin/gencat
SORT   = /bin/sort

default: all

all: diff diff.cat 

diff: ${OBJS} ${HDRS}
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) -o diff $(OBJS) 

clean:
	/bin/rm -f *.o diff.msg 

clobber: clean
	/bin/rm -f diff diff.cat 

diff.cat: diff.msg

diff.msg:
	$(FINDMSG) *.c | $(SORT) -u -n +0 -1 > $@

#
# Redefine the .c.o rule so that it uses $(FLAGS) too
#
.c.o:
	$(CC) -c $(CFLAGS) $(FLAGS) $<

#
# We define two rules, for creating .msg and .cat files.  This is so we
# can let make's internal rules do some work for us.  With these rules,
# we only need to specify that the .cat depends on the .msg(s), and
# make will know to make the .msg from the .c if it needs to.
#
.msg.cat:
	${GENCAT} $*.cat $*.msg

.SUFFIXES: .cat .msg

# copy /bin/diff to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/diff
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	diff
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp diff ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/diff
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
# copy /bin/diff to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/diff
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp diff ../../../Filesets.sl/CMDS-AUX/bin/diff
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/diff
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/diff to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/diff
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp diff.cat ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/diff.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/diff.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
