# HPUX_ID: @(#) $Revision: 70.1.2.2 $

# "make" command file for building the media initialization program mediainit

# COPTS=
# CCOPTS=
CFLAGS=${COPTS}

SOURCES = mi_main.c mi_scsi.c mi_mfinit.c mi_ac.c mi_sio_cs80.c sio_cs80lib.c mi_wsio_cs80.c mi_amigo.c hpux_rel.c
OBJECTS = mi_main.o mi_scsi.o mi_mfinit.o mi_ac.o mi_sio_cs80.o sio_cs80lib.o mi_wsio_cs80.o mi_amigo.o hpux_rel.o
INCLUDES = mi_sio_cs80.h sio_cs80lib.h

default:
	if hp9000s800; then	\
		libs=-lIO;	\
	fi;			\
	$(MAKE) LIBS="$$libs" mediainit mediainit.arch

mediainit:  $(OBJECTS) 
	$(CC) $(LDFLAGS) -o $@ $(LFILES) $(OBJECTS) $(LIBS)

mediainit.arch:  $(OBJECTS) 
	$(CC) $(LDFLAGS) -Wl,-a,archive -o $@ $(LFILES) $(OBJECTS) $(LIBS)

mi_main.o: mi_main.c $(INCLUDES)
	$(CC) -c $(CFLAGS) mi_main.c

mi_scsi.o: mi_scsi.c $(INCLUDES)
	$(CC) -c $(CFLAGS) mi_scsi.c

mi_mfinit.o: mi_mfinit.c $(INCLUDES)
	$(CC) -c $(CFLAGS) mi_mfinit.c

mi_ac.o: mi_ac.c $(INCLUDES)
	$(CC) -c $(CFLAGS) mi_ac.c

mi_sio_cs80.o: mi_sio_cs80.c $(INCLUDES)
	$(CC) -c $(CFLAGS) mi_sio_cs80.c

mi_wsio_cs80.o: mi_wsio_cs80.c $(INCLUDES)
	$(CC) -c $(CFLAGS) mi_wsio_cs80.c

mi_amigo.o: mi_amigo.c $(INCLUDES)
	$(CC) -c $(CFLAGS) mi_amigo.c

hpux_rel.o: hpux_rel.c $(INCLUDES)
	$(CC) -c $(CFLAGS) hpux_rel.c


clean:
	/bin/rm -f $(OBJECTS)

clobber: clean
	/bin/rm -f mediainit mediainit.arch
# copy /usr/bin/mediainit_shared to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/mediainit_shared
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp mediainit ../../../../Filesets.sl/SYS-ADMIN/usr/bin/mediainit
	! chmog 04555 root bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin/mediainit
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mediainit_shared to the . relative fileset INSTALL
# change mode, owner, and group on /usr/bin/mediainit_shared
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/INSTALL/etc
	! cp mediainit.arch ../../../../Filesets.sl/INSTALL/etc/mediainit
	! chmog 04555 root bin ../../../../Filesets.sl/INSTALL/etc/mediainit
	! chmog 555 bin bin ../../../../Filesets.sl/INSTALL/etc
	! chmog 555 bin bin ../../../../Filesets.sl/INSTALL/
	! chmog 555 bin bin ../../../../Filesets.sl/
