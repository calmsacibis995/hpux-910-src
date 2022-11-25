# $Source: /misc/source_product/9.10/commands.rcs/etc/boot/sdsboot.mk,v $
# $Revision: 70.2 $

# Makefile for S300 boot command

FLAGS  = -DSTANDALONE -DACLS -DSDS -DSDS_DEBUG

# location for step b bootstrap
RELOCB=	fff00000

default: sdsboot

# bootable from floppy or real disks
# relsrt0.o must be loaded first

sdsboot: relsrt0.o boot.o hpux_rel.o bootconf.o sds_boot.o libsa.a
	ld -s -a archive -N -R $(RELOCB) -e entry -o sdsboot \
	    relsrt0.o hpux_rel.o boot.o bootconf.o sds_boot.o libsa.a -lc
	chmod 644 sdsboot

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
	$(CC) -c $(CFLAGS) $(FLAGS) $(SFLAGS) conf.c -o $@

boot.o: boot.c
	$(CC) -c $(CFLAGS) $(FLAGS) $(SFLAGS) boot.c -o $@

sds_boot.o: sds_boot.c
	$(CC) -c $(CFLAGS) $(FLAGS) $(SFLAGS) sds_boot.c -o $@

sys.o: sys.c
	$(CC) -c $(CFLAGS) $(FLAGS) $(SFLAGS) sys.c -o $@

driver.o: driver.s
	cc $(FLAGS) $(SFLAGS) -c driver.s

relsrt0.o: srt0.s 
	as -o relsrt0.o srt0.s

libsa.a: sys.o conf.o driver.o
	ar rv libsa.a $?

clean:
	/bin/rm -f *.o
	/bin/rm -f  bootb mkboot sdsboot libsa.a

clobber: clean
	/bin/rm -f boot sdsboot mkrs.boot ins.boot
