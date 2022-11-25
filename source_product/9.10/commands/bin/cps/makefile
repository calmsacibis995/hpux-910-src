default:	cps.sh
	 /bin/cp cps.sh cps
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f cps.sh

# copy /bin/cps to the . relative fileset DISKLESS
# change mode, owner, and group on /bin/cps
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cps
	mkdir -p ../../../Filesets.sl/DISKLESS/bin
	! cp cps ../../../Filesets.sl/DISKLESS/bin
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/bin/cps
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/bin
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
# copy /bin/cps to the . relative fileset DISKLESS
# change mode, owner, and group on /bin/cps
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/DISKLESS/bin
	! cp cps ../../../Filesets.sl/DISKLESS/bin/cps
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/bin/cps
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/bin
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
	! chmog 555 bin bin ../../../Filesets.sl/
