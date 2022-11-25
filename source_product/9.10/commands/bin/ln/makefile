default:	ln.c
	 $(CC)  ln.c -o ln
	 findmsg ln.c > ln.msg
	 gencat ln.cat ln.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f ln

# copy /bin/ln to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/ln
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	ln
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp ln ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/ln
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/ln to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/ln
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp ln ../../../Filesets.sl/UX-CORE/bin/ln
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/ln
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/ln to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/ln
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp ln.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/ln.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/ln.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
