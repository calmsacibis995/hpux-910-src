# $Source: /misc/source_product/9.10/commands.rcs/etc/fpa/makefile,v $
# $Revision: 66.4 $

# Makefile for S300 fpa commands

SRCFILES =	seq.ucode \
		hw.ucode \
		nhw.ucode \
		zero.code

UCODE_ADDR =    0x80000
TABLE_ADDR =    0x10000
TABLE_SIZE =    1024

default:	fpacode fpaload

fpacode:	fpagen $(SRCFILES)
		./fpagen -u$(UCODE_ADDR) -t$(TABLE_ADDR) -s$(TABLE_SIZE) \
		    fpacode $(SRCFILES)

fpagen:		fpagen.c
		$(CC) $(CFLAGS) -Wl,-a,archive $(LDFLAGS) -o fpagen fpagen.c

fpaload:	fpaload.c
		$(CC) $(CFLAGS) $(LDFLAGS) -o fpaload fpaload.c

clean:		
	/bin/rm -f fpagen *.o

clobber: clean
	/bin/rm -f fpaload fpacode

# copy /etc/fpa to the . relative fileset FPA
# change mode, owner, and group on /etc/fpa
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../Filesets.sl/FPA/etc
	! cp fpacode ../../../Filesets.sl/FPA/etc/fpacode
	! chmog 0555 bin bin ../../../Filesets.sl/FPA/etc/fpacode
	! chmog 555 bin bin ../../../Filesets.sl/FPA/etc
	! chmog 555 bin bin ../../../Filesets.sl/FPA/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/fpa to the . relative fileset FPA
# change mode, owner, and group on /etc/fpa
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/FPA/etc
	! cp fpaload ../../../Filesets.sl/FPA/etc/fpaload
	! chmog 0555 bin bin ../../../Filesets.sl/FPA/etc/fpaload
	! chmog 555 bin bin ../../../Filesets.sl/FPA/etc
	! chmog 555 bin bin ../../../Filesets.sl/FPA/
	! chmog 555 bin bin ../../../Filesets.sl/
