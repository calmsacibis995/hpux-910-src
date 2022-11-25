# $Source: /misc/source_product/9.10/commands.rcs/etc/restore/makefile,v $
# $Revision: 72.10 $

# Common makefile for restore

LIBS = -lBSD 
B1 = -lBSD -lscmd -lsec
OBJECTS  =  main.o restore.o dirs.o symtab.o tape.o utilities.o hpux_rel.o \
            interactive.o telldir.o dumprmt.o opendir.o
SOURCES  =  main.c restore.c dirs.c symtab.c tape.c utilities.c hpux_rel.c \
            interactive.c telldir.c ../dump/dumprmt.c opendir.c
INCLUDES = -I../dump

default: restore restore.arch

metrics: restore.M

restore: $(OBJECTS)
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then		\
		$(CC) $(LDFLAGS) $(OBJECTS) -lm6 -o restore $(B1);	\
	else								\
		$(CC) $(LDFLAGS) $(OBJECTS) -o restore $(LIBS);		\
	fi

restore.arch: restore
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then		\
	$(CC) $(LDFLAGS) -Wl,-a,archive $(OBJECTS) -lm6 -o restore.arch $(B1); \
	else								\
	$(CC) $(LDFLAGS) -Wl,-a,archive $(OBJECTS) -o restore.arch $(LIBS); \
	fi

restore.M: $(SOURCES)
	$(METRICS) $(CFLAGS) $(SOURCES) -o restore.M $(LIBS)

clean:
	/bin/rm -f *.o core

clobber: clean
	/bin/rm -f restore restore.arch restore.M

$(OBJECTS): 	restore.h dumprestore.h

dumprmt.o: ../dump/dumprmt.c
	$(CC) $(CFLAGS) $(FLAGS) $(INCLUDES) -c ../dump/dumprmt.c

# Redefine the .c to .o rule as to include FLAGS
.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<


# copy /etc/restore to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/restore
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	restore
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp restore ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/restore
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/restore to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/restore
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp restore ../../../Filesets.sl/SYS-ADMIN/etc/restore
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/restore
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/restore to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/restore
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! ln ../../../Filesets.sl/SYS-ADMIN/etc/restore ../../../Filesets.sl/SYS-ADMIN/etc/rrestore
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
