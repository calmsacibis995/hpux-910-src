# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/m4/makefile,v $
# $Revision: 64.2.1.4 $

# Common makefile for the m4 macro processor.

OBJECTS = m4.o m4ext.o m4macs.o m4y.o hpux_rel.o
LIB	= -ly

default:	m4

m4:	$(OBJECTS)
	$(CC) $(LDFLAGS) -o m4 $(OBJECTS) $(LIB)

clean:
	/bin/rm -f $(OBJECTS)

clobber: clean
	/bin/rm -f m4


# copy /usr/bin/m4 to the . relative fileset PROG-MIN
# change mode, owner, and group on /usr/bin/m4
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	m4
	mkdir -p ../../../../Filesets.sl/PROG-MIN/usr/bin
	! cp m4 ../../../../Filesets.sl/PROG-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-MIN/usr/bin/m4
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-MIN/
# copy /usr/bin/m4 to the . relative fileset PROG-MIN
# change mode, owner, and group on /usr/bin/m4
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-MIN/usr/bin
	! cp m4 ../../../../Filesets.sl/PROG-MIN/usr/bin/m4
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-MIN/usr/bin/m4
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
