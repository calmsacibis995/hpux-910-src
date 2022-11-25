default:	mount.c
	 $(CC) -Wl,-a,archive -DCDCASE  mount.c -o mount
	 findmsg mount.c > mount.msg
	 gencat mount.cat mount.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f mount

# copy /etc/mount to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/mount
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	mount
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp mount ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/mount
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/mount to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/mount
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp mount ../../../Filesets.sl/UX-CORE/etc/mount
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/mount
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
