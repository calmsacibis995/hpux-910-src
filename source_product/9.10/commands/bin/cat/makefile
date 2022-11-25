default:	cat.c
	 $(CC)  cat.c -o cat
	 findmsg cat.c > cat.msg
	 gencat cat.cat cat.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f cat

# copy /bin/cat to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/cat
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cat
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp cat ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/cat to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/cat
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp cat ../../../Filesets.sl/UX-CORE/bin/cat
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/cat to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/cat
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp cat.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/cat.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/cat.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
