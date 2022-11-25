# $Source: /misc/source_product/9.10/commands.rcs/bin/find/makefile,v $
# $Revision: 70.1.1.5 $

# Common makefile for find

FINDMSG = /usr/bin/findmsg
GENCAT  = /usr/bin/gencat
OBJECTS = find.o funcs.o cpio.o libcpio.o

default: find find.cat

find : $(OBJECTS) hpux_rel.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o find hpux_rel.o $(OBJECTS) -lBUILD

find.cat: find.c
	$(FINDMSG) find.c > find.msg
	$(FINDMSG) cpio.c > cpio.msg
	$(GENCAT)  $@ find.msg cpio.msg

clean:
	/bin/rm -f *.o *.msg

clobber: clean
	/bin/rm -f find find.cat

# copy /bin/find to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/find
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	find
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp find ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/find
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /bin/find to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/find
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp find ../../../Filesets.sl/CMDS-MIN/bin/find
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/find
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/find to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/find
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp find.cat ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/find.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/find.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
