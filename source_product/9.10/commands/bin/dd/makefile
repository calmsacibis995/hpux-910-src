default:	dd.c
	 $(CC)  dd.c -o dd
	 findmsg dd.c > dd.msg
	 gencat dd.cat dd.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f dd

# copy /bin/dd to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/dd
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	dd
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp dd ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/dd
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/dd to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/dd
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp dd ../../../Filesets.sl/UX-CORE/bin/dd
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/dd
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/dd to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/dd
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp dd.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/dd.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/dd.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
