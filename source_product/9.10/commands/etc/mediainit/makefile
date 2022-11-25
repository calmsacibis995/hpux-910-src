# $Source: /misc/source_product/9.10/commands.rcs/etc/mediainit/makefile,v $
# $Revision: 66.5 $

# S300 makefile for mediainit command.  Has been cleaned up to work in
# conjunction with the common build process.

OBJECTS = hpux_rel.o mi_main.o mi_cs80.o mi_ac.o mi_mfinit.o mi_amigo.o mi_scsi.o mi_ac.o

default:  mediainit mediainit.arch

mediainit:  $(OBJECTS) 
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

mediainit.arch:  $(OBJECTS) 
	$(CC) $(LDFLAGS) -Wl,-a,archive -o $@ $(OBJECTS) $(LIBS)

$(OBJECTS):  $(INCLUDES)

clean:
	/bin/rm -f $(OBJECTS)

clobber: clean
	/bin/rm -f mediainit mediainit.arch

# copy /etc/mediainit to the . relative fileset INSTALL
# change mode, owner, and group on /etc/mediainit
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.

install:	mediainit
	mkdir -p ../../../Filesets.sl/INSTALL/etc
	! cp mediainit ../../../Filesets.sl/INSTALL/etc
	! chmog 04555 root bin ../../../Filesets.sl/INSTALL/etc/mediainit
	! chmog 555 bin bin ../../../Filesets.sl/INSTALL/etc
	! chmog 555 bin bin ../../../Filesets.sl/INSTALL/
