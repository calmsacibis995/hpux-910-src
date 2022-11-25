default:	clustersh.c
	 $(CC)  clustersh.c -o clustersh
	 findmsg clustersh.c > clustersh.msg
	 gencat clustersh.cat clustersh.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f clustersh

# copy /etc/clustersh to the . relative fileset DISKLESS
# change mode, owner, and group on /etc/clustersh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	clustersh
	mkdir -p ../../../Filesets.sl/DISKLESS/etc
	! cp clustersh ../../../Filesets.sl/DISKLESS/etc
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/etc/clustersh
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/etc
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
# copy /etc/clustersh to the . relative fileset DISKLESS
# change mode, owner, and group on /etc/clustersh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/DISKLESS/etc
	! cp clustersh ../../../Filesets.sl/DISKLESS/etc/clustersh
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/etc/clustersh
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/etc
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
	! chmog 555 bin bin ../../../Filesets.sl/
