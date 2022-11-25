default:	pwd.c
	 $(CC)  pwd.c -o pwd
	 findmsg pwd.c > pwd.msg
	 gencat pwd.cat pwd.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f pwd

# copy /bin/pwd to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/pwd
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	pwd
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp pwd ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/pwd
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/pwd to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/pwd
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp pwd ../../../Filesets.sl/UX-CORE/bin/pwd
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/pwd
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/pwd to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/pwd
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp pwd.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/pwd.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/pwd.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
