default:	mt.c
	 $(CC)  mt.c -o mt
	 findmsg mt.c > mt.msg
	 gencat mt.cat mt.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f mt

# copy /bin/mt to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /bin/mt
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	mt
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/bin
	! cp mt ../../../Filesets.sl/SYS-ADMIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/bin/mt
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /bin/mt to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /bin/mt
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/bin
	! cp mt ../../../Filesets.sl/SYS-ADMIN/bin/mt
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/bin/mt
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
