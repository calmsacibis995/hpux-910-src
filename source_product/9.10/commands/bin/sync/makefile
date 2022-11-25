default:	sync.c
	 $(CC) -Wl,-a,archive  sync.c -o sync
	 findmsg sync.c > sync.msg
	 gencat sync.cat sync.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f sync

# copy /bin/sync to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sync
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	sync
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp sync ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/sync
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/sync to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sync
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp sync ../../../Filesets.sl/UX-CORE/bin/sync
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/sync
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
