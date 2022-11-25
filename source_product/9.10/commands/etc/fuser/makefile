# $Source: /misc/source_product/9.10/commands.rcs/etc/fuser/makefile,v $
# $Revision: 66.5.1.4 $

# makefile for S300 fuser 

CFLAGS=	$(CCOPTS)
default: fuser

fuser:  fuser.c
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then	\
	    libs="$$libs -lscmd -lsec";				\
	fi;							\
	$(CC) $(CFLAGS) $(LDFLAGS) -o fuser fuser.c $$libs

clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f fuser

# copy /etc/fuser to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/fuser
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	fuser
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp fuser ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0544 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/fuser
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/fuser to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/fuser
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp fuser ../../../Filesets.sl/SYS-ADMIN/etc/fuser
	! chmog 0544 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/fuser
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
