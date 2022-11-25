# @(#) $Revision: 1.3.84.3 $    

ROOT    =
INSDIR  = $(ROOT)/etc
CFLAGS  = -O -DSTANDALONE

# location for step b bootstrap
RELOCB=	ffff0800

all: boot

# bootable from floppy or real disks
# relsrt0.o must be loaded first

boot:   depend mkboot boot.o hpux_rel.o relsrt0.o bootconf.o libsa.a
	ld -N -R $(RELOCB) -e entry -o bootb relsrt0.o hpux_rel.o boot.o bootconf.o libsa.a -lc
	./mkboot -i bootb -o boot -n SYSHPUX SYSDEBUG SYSBCKUP

mkboot:
	cc.unshared mkboot.c -o mkboot

bootconf.o: conf.c /usr/include/sys/param.h /usr/include/sys/inode.h
bootconf.o: /usr/include/sys/fs.h saio.h
	cp conf.c bootconf.c
	cc -c $(CFLAGS) bootconf.c
	rm bootconf.c

# startups

relsrt0.o: srt0.s 
	as -o relsrt0.o srt0.s

libsa.a: sys.o conf.o driver.o
	ar rv libsa.a $?

clean:
	-rm -f *.o
	-rm -f  bootb

clobber:
	-rm -f boot

install: release

release: boot
	cp    boot      $(INSDIR)/boot
	chown root      $(INSDIR)/boot
	chgrp other     $(INSDIR)/boot
	chmod 444       $(INSDIR)/boot

depend: /usr/include/sys/param.h \
	/usr/include/sys/inode.h \
	/usr/include/sys/fs.h \
	/usr/include/a.out.h \
	/usr/include/sys/dir.h \
	saio.h
