default:	passwd.c
	 mv passwd passwd.file
	 $(CC)  passwd.c -o passwd
	 findmsg passwd.c > passwd.msg
	 gencat passwd.cat passwd.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f passwd
# copy /etc/passwd to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/passwd
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp passwd ../../../Filesets.sl/UX-CORE/bin/passwd
	! chmog 04555 root bin ../../../Filesets.sl/UX-CORE/bin/passwd
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/passwd to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/passwd
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp passwd.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/passwd.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/passwd.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
	mkdir -p ../../../Filesets.sl/UX-CORE/etc/newconfig
	! cp passwd.file ../../../Filesets.sl/UX-CORE/etc/newconfig/passwd
	! chmog 444 root other ../../../Filesets.sl/UX-CORE/etc/newconfig/passwd
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc/newconfig
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE
	! chmog 555 bin bin ../../../Filesets.sl
