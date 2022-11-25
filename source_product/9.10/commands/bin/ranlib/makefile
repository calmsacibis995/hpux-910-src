default:	ranlib.sh
	 /bin/cp ranlib.sh ranlib
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f ranlib

# copy /bin/ranlib to the . relative fileset BUILDENV
# change mode, owner, and group on /bin/ranlib
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	ranlib
	mkdir -p ../../../Filesets.sl/BUILDENV/bin
	! cp ranlib ../../../Filesets.sl/BUILDENV/bin
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/bin/ranlib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/bin
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
# copy /bin/ranlib to the . relative fileset BUILDENV
# change mode, owner, and group on /bin/ranlib
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/BUILDENV/bin
	! cp ranlib ../../../Filesets.sl/BUILDENV/bin/ranlib
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/bin/ranlib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/bin
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/ranlib to the . relative fileset C-MIN
# change mode, owner, and group on /bin/ranlib
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C-MIN/bin
	! cp ranlib ../../../Filesets.sl/C-MIN/bin/ranlib
	! chmog 0555 bin bin ../../../Filesets.sl/C-MIN/bin/ranlib
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
