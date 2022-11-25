default:	ccck.c
	$(CC) ccck.c -o ccck

# copy /etc/ccck to the . relative fileset DISKLESS
# change mode, owner, and group on /etc/ccck
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	ccck
	mkdir -p ../../../Filesets.sl/DISKLESS/etc
	! cp ccck ../../../Filesets.sl/DISKLESS/etc
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/etc/ccck
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/etc
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
# copy /etc/ccck to the . relative fileset DISKLESS
# change mode, owner, and group on /etc/ccck
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/DISKLESS/etc
	! cp ccck ../../../Filesets.sl/DISKLESS/etc/ccck
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/etc/ccck
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/etc
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
	! chmog 555 bin bin ../../../Filesets.sl/
