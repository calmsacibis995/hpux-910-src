# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/bs/makefile,v $
# $Revision: 64.2.1.4 $

# Common makefile for bs

OFILES = atof.o bs.o string.o hpux_rel.o

default : bs

bs: $(OFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o bs $(OFILES) -lm

atof.o: atof.c

bs.o: bs.c 

string.o: string.c

# 	The test target apparently tests the bs executable in some way.
#	Since the file "testall" is in shared source and someone might know
#	what it does, this target stays for now.
test:
	bs testall

clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f bs

# copy /usr/bin/bs to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/bs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	bs
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp bs ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/bs
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/bs to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/bs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp bs ../../../../Filesets.sl/CMDS-AUX/usr/bin/bs
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/bs
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
