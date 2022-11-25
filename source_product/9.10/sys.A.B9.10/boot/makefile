# $Source: /source/hpux_source/kernel/sys.SWT68K_300/boot/RCS/makefile,v $
# $Revision: 1.3.84.4 $

# Makefile for S300 boot command

FLAGS  = -DSTANDALONE
CFLAGS=-I../Filesets/BUILDENV/usr/include

# location for step b bootstrap
RELOCB=	ffff0800

default: boot

# bootable from floppy or real disks
# relsrt0.o must be loaded first

boot: mkboot bootb
	./mkboot -i bootb -o boot -n SYSHPUX SYSDEBUG SYSBCKUP

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

relsrt0.o: srt0.s 
	as -o relsrt0.o srt0.s

libsa.a: sys.o conf.o driver.o
	ar rv libsa.a $?

clean:
	/bin/rm -f *.o
	/bin/rm -f  bootb

clobber: clean
	/bin/rm -f boot
