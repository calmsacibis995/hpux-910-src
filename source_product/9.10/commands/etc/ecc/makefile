# $Source: /misc/source_product/9.10/commands.rcs/etc/ecc/makefile,v $
# @(#) $Revision: 64.4 $

# "make" command file for building ecclogger and eccscrub.

LOGOBJECTS =	ecclogger.o 
SCRUBOBJECTS =	eccscrub.o cas.o
VERSION = hpux_rel

default:  ecclogger eccscrub

ecclogger:	$(LOGOBJECTS) $(VERSION).o
		$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(LOGOBJECTS) $(VERSION).o

eccscrub:	$(SCRUBOBJECTS) $(VERSION.o)
		$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SCRUBOBJECTS) $(VERSION).o

clean:
	/bin/rm -f $(LOGOBJECTS) $(SCRUBOBJECTS) $(VERSION).o

clobber: clean
	/bin/rm -f ecclogger eccscrub
# copy /etc/ecc to the . relative fileset ECC-TOOLS
# change mode, owner, and group on /etc/ecc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../Filesets.sl/ECC-TOOLS/etc
	! cp ecclogger ../../../Filesets.sl/ECC-TOOLS/etc/ecclogger
	! chmog 0555 bin bin ../../../Filesets.sl/ECC-TOOLS/etc/ecclogger
	! chmog 555 bin bin ../../../Filesets.sl/ECC-TOOLS/etc
	! chmog 555 bin bin ../../../Filesets.sl/ECC-TOOLS/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/ecc to the . relative fileset ECC-TOOLS
# change mode, owner, and group on /etc/ecc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/ECC-TOOLS/etc
	! cp eccscrub ../../../Filesets.sl/ECC-TOOLS/etc/eccscrub
	! chmog 0555 bin bin ../../../Filesets.sl/ECC-TOOLS/etc/eccscrub
	! chmog 555 bin bin ../../../Filesets.sl/ECC-TOOLS/etc
	! chmog 555 bin bin ../../../Filesets.sl/ECC-TOOLS/
	! chmog 555 bin bin ../../../Filesets.sl/
