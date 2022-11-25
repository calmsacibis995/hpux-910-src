default:	gen_proto.c
	 $(CC)  gen_proto.c -o gen_proto
	 findmsg gen_proto.c > gen_proto.msg
	 gencat gen_proto.cat gen_proto.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f gen_proto

# copy /usr/lib/gen_proto to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/lib/gen_proto
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	gen_proto
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/lib
	! cp gen_proto ../../../../Filesets.sl/SYS-ADMIN/usr/lib
	! chmog 0555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/lib/gen_proto
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
# copy /usr/lib/gen_proto to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/lib/gen_proto
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/lib
	! cp gen_proto ../../../../Filesets.sl/SYS-ADMIN/usr/lib/gen_proto
	! chmog 0555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/lib/gen_proto
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
