default:	audomon.c
	 $(CC)  audomon.c -o audomon
	 findmsg audomon.c > audomon.msg
	 gencat audomon.cat audomon.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f audomon


# copy /etc/audomon to the . relative fileset AUDIT
# change mode, owner, and group on /etc/audomon
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	audomon
	mkdir -p ../../../Filesets.sl/AUDIT/etc
	! cp audomon ../../../Filesets.sl/AUDIT/etc
	! chmog 0555 bin bin ../../../Filesets.sl/AUDIT/etc/audomon
	! chmog 555 bin bin ../../../Filesets.sl/AUDIT/etc
	! chmog 555 bin bin ../../../Filesets.sl/AUDIT/
# copy /etc/audomon to the . relative fileset AUDIT
# change mode, owner, and group on /etc/audomon
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/AUDIT/etc
	! cp audomon ../../../Filesets.sl/AUDIT/etc/audomon
	! chmog 0555 bin bin ../../../Filesets.sl/AUDIT/etc/audomon
	! chmog 555 bin bin ../../../Filesets.sl/AUDIT/etc
	! chmog 555 bin bin ../../../Filesets.sl/AUDIT/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/audomon to the . relative fileset AUDIT
# change mode, owner, and group on /etc/audomon
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/AUDIT/usr/lib/nls/C
	! cp audomon.cat ../../../Filesets.sl/AUDIT/usr/lib/nls/C/audomon.cat
	! chmog 0444 bin bin ../../../Filesets.sl/AUDIT/usr/lib/nls/C/audomon.cat
	! chmog 555 bin bin ../../../Filesets.sl/AUDIT/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/AUDIT/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/AUDIT/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/AUDIT/usr/
	! chmog 555 bin bin ../../../Filesets.sl/AUDIT/
	! chmog 555 bin bin ../../../Filesets.sl/
