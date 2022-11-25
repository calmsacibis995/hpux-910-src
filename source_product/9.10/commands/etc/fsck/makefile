# $Source: /misc/source_product/9.10/commands.rcs/etc/fsck/makefile,v $
# $Revision: 66.1.1.6 $

# Common makefile for fsck

OBJS = fsck.o ufs_tables.o ufs_subr.o hpux_rel.o

default: all

all: $(OBJS)
	libs="";						\
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then	\
	    libs="$$libs -lscmd -lsec -lcurses";		\
	fi;							\
	$(CC) -Wl,-a,archive -DCACHE -DFIND_SB $(CFLAGS) $(LDFLAGS) -o fsck $(OBJS) $$libs

clean: 
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f fsck

fsck.c: wait.h

fsck.o:	fsck.c
	$(CC) -DCACHE -DFIND_SB $(CFLAGS) -c fsck.c

# copy /etc/fsck to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/fsck
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	fsck
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp fsck ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/fsck
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/fsck to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/fsck
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp fsck ../../../Filesets.sl/UX-CORE/etc/fsck
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/fsck
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
