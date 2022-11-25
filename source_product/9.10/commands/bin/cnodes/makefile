default:	cnodes.c
	 $(CC) cnodes.c -o cnodes -lcurses
	 findmsg cnodes.c > cnodes.msg
	 gencat cnodes.cat cnodes.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f cnodes

# copy /bin/cnodes to the . relative fileset DISKLESS
# change mode, owner, and group on /bin/cnodes
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cnodes
	mkdir -p ../../../Filesets.sl/DISKLESS/bin
	! cp cnodes ../../../Filesets.sl/DISKLESS/bin
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/bin/cnodes
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/bin
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
# copy /bin/cnodes to the . relative fileset DISKLESS
# change mode, owner, and group on /bin/cnodes
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/DISKLESS/bin
	! cp cnodes ../../../Filesets.sl/DISKLESS/bin/cnodes
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/bin/cnodes
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/bin
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/cnodes to the . relative fileset DISKLESS
# change mode, owner, and group on /bin/cnodes
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/DISKLESS/usr/lib/nls/C
	! cp cnodes.cat ../../../Filesets.sl/DISKLESS/usr/lib/nls/C/cnodes.cat
	! chmog 0444 bin bin ../../../Filesets.sl/DISKLESS/usr/lib/nls/C/cnodes.cat
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/usr/
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
	! chmog 555 bin bin ../../../Filesets.sl/
