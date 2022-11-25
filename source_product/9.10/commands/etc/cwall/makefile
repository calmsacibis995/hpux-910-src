default:	cwall.sh
	 /bin/cp cwall.sh cwall
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f cwall.sh

# copy /etc/cwall to the . relative fileset DISKLESS
# change mode, owner, and group on /etc/cwall
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cwall
	mkdir -p ../../../Filesets.sl/DISKLESS/etc
	! cp cwall ../../../Filesets.sl/DISKLESS/etc
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/etc/cwall
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/etc
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
# copy /etc/cwall to the . relative fileset DISKLESS
# change mode, owner, and group on /etc/cwall
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/DISKLESS/etc
	! cp cwall ../../../Filesets.sl/DISKLESS/etc/cwall
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/etc/cwall
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/etc
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
	! chmog 555 bin bin ../../../Filesets.sl/
