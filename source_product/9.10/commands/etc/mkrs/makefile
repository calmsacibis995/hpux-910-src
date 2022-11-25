# $Source: /misc/source_product/9.10/commands.rcs/etc/mkrs/makefile,v $
# $Revision: 70.1.1.7 $

BOOTLF = /usr/lib/uxbootlf.700

OBJECTS = mkrs.tool.o   \
	  mkrs.init.o

default: objects mkrs.boot

objects: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) mkrs.tool.o  -o mkrs.tool
	$(CC) $(CFLAGS) $(LDFLAGS) mkrs.init.o  -o mkrs.init

mkrs.boot: 
	if hp9000s700; then \
	   lifcp -r $(BOOTLF)":ISL" isl; \
	   strip isl; \
	   lifinit `iplopt isl` -v20000000 -d 16 -nISL10 -K2 -s4096 mkrs.boot; \
	   lifcp -r -T-12800 -K2 isl mkrs.boot:ISL; \
	   echo "hpux boot tape(;0):RECOVER" > auto_file; \
	   lifcp -r -T-12289 -K2 auto_file mkrs.boot:AUTO; \
	   lifcp -r -T-12928 -K2 $(BOOTLF)":HPUX" mkrs.boot:HPUX; \
	   rm -f isl auto_file; \
	fi;

clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f mkrs.tool mkrs.init mkrs.boot

# copy /etc/mkrs to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/mkrs
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	mkrs
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp mkrs ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0544 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/mkrs
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/mkrs to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/mkrs
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp mkrs ../../../Filesets.sl/SYS-ADMIN/etc/mkrs
	! chmog 0544 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/mkrs
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/mkrs to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/mkrs
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp mkrs.data ../../../Filesets.sl/SYS-ADMIN/etc/mkrs.data
	! chmog 0444 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/mkrs.data
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/mkrs to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/mkrs
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp mkrs.init ../../../Filesets.sl/SYS-ADMIN/etc/mkrs.init
	! chmog 0544 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/mkrs.init
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/mkrs to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/mkrs
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp mkrs.tool ../../../Filesets.sl/SYS-ADMIN/etc/mkrs.tool
	! chmog 0544 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/mkrs.tool
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
