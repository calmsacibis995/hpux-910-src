default:	su.c
	 $(CC) -Wl,-a,archive  su.c -o su
	 findmsg su.c > su.msg
	 gencat su.cat su.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f su

# copy /bin/su to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/su
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.

install:	su
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp su ../../../Filesets.sl/UX-CORE/bin
	! chmog 04555 root bin ../../../Filesets.sl/UX-CORE/bin/su
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/su to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/su
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp su ../../../Filesets.sl/UX-CORE/bin/su
	! chmog 04555 root bin ../../../Filesets.sl/UX-CORE/bin/su
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/su to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/su
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp su.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/su.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/su.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
