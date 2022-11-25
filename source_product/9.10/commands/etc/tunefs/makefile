default:	tunefs.c
	 $(CC)  tunefs.c -o tunefs
	 findmsg tunefs.c > tunefs.msg
	 gencat tunefs.cat tunefs.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f tunefs

# copy /etc/tunefs to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/tunefs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	tunefs
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp tunefs ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/tunefs
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/tunefs to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/tunefs
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp tunefs ../../../Filesets.sl/SYS-ADMIN/etc/tunefs
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/tunefs
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
