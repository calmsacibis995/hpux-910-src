default:	cluster.c
	 $(CC) cluster.c -o cluster -lDUX
	 findmsg cluster.c > cluster.msg
	 gencat cluster.cat cluster.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f cluster

# copy /etc/cluster to the . relative fileset DISKLESS
# change mode, owner, and group on /etc/cluster
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cluster
	mkdir -p ../../../Filesets.sl/DISKLESS/etc
	! cp cluster ../../../Filesets.sl/DISKLESS/etc
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/etc/cluster
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/etc
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
# copy /etc/cluster to the . relative fileset DISKLESS
# change mode, owner, and group on /etc/cluster
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/DISKLESS/etc
	! cp cluster ../../../Filesets.sl/DISKLESS/etc/cluster
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/etc/cluster
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/etc
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
	! chmog 555 bin bin ../../../Filesets.sl/
