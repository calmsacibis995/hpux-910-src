# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/file/makefile,v $
# $Revision: 66.1.1.5 $

#
#	Makefile for file command -- necessary to install /etc/magic
#
default:	file

file:	file.o hpux_rel.o
	$(CC) $(CFLAGS) $(LDFLAGS) file.o hpux_rel.o -o file

clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f file

.c.o:
	$(CC) $(CFLAGS) -c $<

# copy /usr/bin/file to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/file
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	file
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp file ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/file
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/file to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/file
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/etc/newconfig
	! cp magic ../../../../Filesets.sl/CMDS-AUX/etc/newconfig/magic
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/etc/newconfig/magic
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/etc/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/etc/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/file to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/file
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp file ../../../../Filesets.sl/CMDS-AUX/usr/bin/file
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/file
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
