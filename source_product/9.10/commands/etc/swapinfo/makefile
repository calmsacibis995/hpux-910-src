default:	swapinfo.c
	 $(CC)  swapinfo.c -o swapinfo
	 findmsg swapinfo.c > swapinfo.msg
	 gencat swapinfo.cat swapinfo.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f swapinfo

# copy /etc/swapinfo to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/swapinfo
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	swapinfo
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp swapinfo ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0544 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/swapinfo
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/swapinfo to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/swapinfo
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp swapinfo ../../../Filesets.sl/SYS-ADMIN/etc/swapinfo
	! chmog 0544 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/swapinfo
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
