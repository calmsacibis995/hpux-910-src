default:	comm.c
	 $(CC)  comm.c -o comm
	 findmsg comm.c > comm.msg
	 gencat comm.cat comm.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f comm

# copy /usr/bin/comm to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/bin/comm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	comm
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/bin
	! cp comm ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin/comm
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
# copy /usr/bin/comm to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/bin/comm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/bin
	! cp comm ../../../../Filesets.sl/UX-CORE/usr/bin/comm
	! chmog 0555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin/comm
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/comm to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/bin/comm
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp comm.cat ../../../../Filesets.sl/UX-CORE/usr/lib/nls/C/comm.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/UX-CORE/usr/lib/nls/C/comm.cat
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
