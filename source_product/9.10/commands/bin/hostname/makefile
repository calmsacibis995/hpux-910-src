default:	hostname.c
	 $(CC)  hostname.c -o hostname
	 findmsg hostname.c > hostname.msg
	 gencat hostname.cat hostname.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f hostname

# copy /bin/hostname to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/hostname
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	hostname
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp hostname ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/hostname
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/hostname to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/hostname
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp hostname ../../../Filesets.sl/UX-CORE/bin/hostname
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/hostname
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
