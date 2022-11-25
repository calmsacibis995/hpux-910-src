default:
	cp cfuser.sh cfuser

# copy /etc/cfuser to the . relative fileset DISKLESS
# change mode, owner, and group on /etc/cfuser
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cfuser
	mkdir -p ../../../Filesets.sl/DISKLESS/etc
	! cp cfuser ../../../Filesets.sl/DISKLESS/etc
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/etc/cfuser
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/etc
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
# copy /etc/cfuser to the . relative fileset DISKLESS
# change mode, owner, and group on /etc/cfuser
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/DISKLESS/etc
	! cp cfuser ../../../Filesets.sl/DISKLESS/etc/cfuser
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/etc/cfuser
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/etc
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
	! chmog 555 bin bin ../../../Filesets.sl/
