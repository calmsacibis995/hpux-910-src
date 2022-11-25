# $Source: /misc/source_product/9.10/commands.rcs/bin/ed/makefile,v $
# $Revision: 66.1.1.6 $

# Makefile for the ed editor
#

FLAGS   = -UNULLS -URESEARCH -DS5R2
FINDMSG	= /usr/bin/findmsg
GENCAT	= /usr/bin/gencat
TOUCH	= /bin/touch

default: all

all:	ed ed.cat

ed:	ed.o ed_crypt.o hpux_rel.o
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) ed.o ed_crypt.o hpux_rel.o -o ed

# For some engineers, CRYPT-specific source is unavailable.  Touch the file
# so that the (non-CRYPT) make can still work.
#
ed_crypt.o:
	$(TOUCH) ed_crypt.c
	$(CC) $(CFLAGS) $(FLAGS) -c ed_crypt.c

clean:
	/bin/rm -f *.o *.msg

clobber: clean
	/bin/rm -f ed ed.cat

ed.cat:	ed.msg

#
# We re-define the .c.o rule so that implicit make rules to compile .o's
# will compile using the FLAGS macro
#
.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<

#
# We define two rules for .msg and .cat files so make will implicitly do
# the work for us.  With these rules, we only need to specify that the 
# .cat depends on the .msg.  Make will then know to make the .msg from the
# .c if it needs to, and knows how to make the .cat from the .msg
#
.c.msg:
	$(FINDMSG) $*.c >$*.msg

.msg.cat:
	$(GENCAT) $*.cat $*.msg

.SUFFIXES: .cat .msg

# copy /bin/ed to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/ed
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	ed
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp ed ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/ed
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /bin/ed to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/ed
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp ed ../../../Filesets.sl/CMDS-MIN/bin/ed
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/ed
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/ed to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/ed
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp ed.cat ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/ed.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/ed.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/ed to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/ed
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! ln ../../../Filesets.sl/CMDS-MIN/bin/ed ../../../Filesets.sl/CMDS-MIN/bin/red
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
