# $Source: /misc/source_product/9.10/commands.rcs/etc/boot/makefile,v $
# $Revision: 70.10 $

# Makefile for S300 boot command

FLAGS  = -DSTANDALONE -DACLS

# location for step b bootstrap
RELOCB=	ffff0800

default: boot

# bootable from floppy or real disks
# relsrt0.o must be loaded first

boot: mkboot bootb
#	./mkboot -i bootb -o boot -n SYSHPUX SYSDEBUG SYSBCKUP SYSTEST \
#				     SYSXDB SYXTERM # For XTERM local boot.
	./mkboot -i bootb -o boot -n SYSHPUX SYSDEBUG SYSBCKUP SYSTEST SYSXDB
	./mkboot -i bootb -o mkrs.boot -n SYSRECOVER
	./mkboot -i bootb -o ins.boot -n SYSINSTALL
	make clean
	make -f sdsboot.mk

bootb: relsrt0.o boot.o hpux_rel.o bootconf.o libsa.a
	ld -s -a archive -N -R $(RELOCB) -e entry -o bootb \
	    relsrt0.o hpux_rel.o boot.o bootconf.o libsa.a -lc

#
# In a previous life, this makefile used cc.unshared to compile mkboot
# rather that $(CC).  This was because certain shared libraries would
# not work on some systems, so cc.unshared used unshared libraries.
#
mkboot: mkboot.o
	$(CC) -Wl,-a,archive mkboot.o -o mkboot

bootconf.o: saio.h
	$(CC) -c $(CFLAGS) $(FLAGS) conf.c -o $@

# startups

conf.o: conf.c
	$(CC) -c $(CFLAGS) $(FLAGS) conf.c -o $@

boot.o: boot.c
	$(CC) -c $(CFLAGS) $(FLAGS) boot.c -o $@

sys.o: sys.c
	$(CC) -c $(CFLAGS) $(FLAGS) sys.c -o $@

driver.o: driver.s
	cc $(FLAGS) -c driver.s

relsrt0.o: srt0.s 
	as -o relsrt0.o srt0.s

libsa.a: sys.o conf.o driver.o
	ar rv libsa.a $?

clean:
	/bin/rm -f *.o
	/bin/rm -f  bootb mkboot sdsbootb libsa.a

clobber: clean
	/bin/rm -f boot sdsboot mkrs.boot ins.boot

# copy /etc/boot to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/boot
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp boot ../../../Filesets.sl/UX-CORE/etc
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/etc/boot
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/boot to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/boot
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp boot ../../../Filesets.sl/UX-CORE/etc/boot
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/etc/boot
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/boot to the . relative fileset INSTALL
# change mode, owner, and group on /etc/boot
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/INSTALL/etc
	! cp ins.boot ../../../Filesets.sl/INSTALL/etc/ins.boot
	! chmog 0444 bin bin ../../../Filesets.sl/INSTALL/etc/ins.boot
	! chmog 555 bin bin ../../../Filesets.sl/INSTALL/etc
	! chmog 555 bin bin ../../../Filesets.sl/INSTALL/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/boot to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/boot
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp mkrs.boot ../../../Filesets.sl/SYS-ADMIN/etc/mkrs.boot
	! chmog 0444 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/mkrs.boot
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/boot to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/boot
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp sdsboot ../../../Filesets.sl/SYS-ADMIN/etc/sdsboot
	! chmog 0444 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/sdsboot
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
