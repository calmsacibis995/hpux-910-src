default:	rm.c
	 $(CC) -Wl,-a,archive  rm.c -o rm
	 findmsg rm.c > rm.msg
	 gencat rm.cat rm.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f rm

# copy /bin/rm to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/rm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	rm
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp rm ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/rm
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/rm to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/rm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp rm ../../../Filesets.sl/UX-CORE/bin/rm
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/rm
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/rm to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/rm
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp rm.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/rm.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/rm.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/rm to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/rm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! ln ../../../Filesets.sl/UX-CORE/bin/rm ../../../Filesets.sl/UX-CORE/bin/rmdir
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
