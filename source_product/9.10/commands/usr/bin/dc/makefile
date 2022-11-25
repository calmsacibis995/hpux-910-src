# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/dc/makefile,v $
# $Revision: 64.2.1.4 $

# Makefile for dc.  We need to compile with hpux_rel.o in case the dc.h
# was updated, keeping the version string correct.

OBJS = dc.o hpux_rel.o

default:	dc

dc:	$(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

clean:
	/bin/rm -f $(OBJS)

clobber: clean
	  /bin/rm -f dc

# copy /usr/bin/dc to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/dc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	dc
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp dc ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/dc
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/dc to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/dc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp dc ../../../../Filesets.sl/CMDS-MIN/usr/bin/dc
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/dc
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
