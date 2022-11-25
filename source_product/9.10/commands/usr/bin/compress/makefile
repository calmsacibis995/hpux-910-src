# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/compress/makefile,v $
# $Revision: 66.2.1.10 $

# Common makefile for compress

default: all

all :	compress uncompressdir

compress : compress.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ compress.o -lBUILD

uncompressdir :
	[ ! -f uncompressdir ] && mv uncompressdi uncompressdir

clean:
	rm -f *.o

clobber: clean
	/bin/rm -f compress

# copy /usr/bin/compress to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/compress
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	compress
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp compress ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/compress
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/compress to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/compress
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp compress ../../../../Filesets.sl/CMDS-MIN/usr/bin/compress
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/compress
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/compress to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/compress
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp compressdir ../../../../Filesets.sl/CMDS-MIN/usr/bin/compressdir
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/compressdir
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/compress to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/compress
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp uncompressdir ../../../../Filesets.sl/CMDS-MIN/usr/bin/uncompressdir
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/uncompressdir
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/compress to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/compress
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp zcmp ../../../../Filesets.sl/CMDS-MIN/usr/bin/zcmp
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/zcmp
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/compress to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/compress
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp zdiff ../../../../Filesets.sl/CMDS-MIN/usr/bin/zdiff
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/zdiff
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/compress to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/compress
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! ln ../../../../Filesets.sl/CMDS-MIN/usr/bin/compress ../../../../Filesets.sl/CMDS-MIN/usr/bin/uncompress
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/compress to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/compress
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! ln ../../../../Filesets.sl/CMDS-MIN/usr/bin/compress ../../../../Filesets.sl/CMDS-MIN/usr/bin/zcat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
