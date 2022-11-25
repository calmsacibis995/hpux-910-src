default:	read_cct.c
	 $(CC)  read_cct.c -o read_cct
	 findmsg read_cct.c > read_cct.msg
	 gencat read_cct.cat read_cct.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f read_cct

# copy /etc/read_cct to the . relative fileset DISKLESS
# change mode, owner, and group on /etc/read_cct
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	read_cct
	mkdir -p ../../../Filesets.sl/DISKLESS/etc
	! cp read_cct ../../../Filesets.sl/DISKLESS/etc
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/etc/read_cct
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/etc
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
# copy /etc/read_cct to the . relative fileset DISKLESS
# change mode, owner, and group on /etc/read_cct
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/DISKLESS/etc
	! cp read_cct ../../../Filesets.sl/DISKLESS/etc/read_cct
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/etc/read_cct
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/etc
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
	! chmog 555 bin bin ../../../Filesets.sl/
