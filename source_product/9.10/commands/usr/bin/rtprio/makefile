default:	rtprio.c
	 $(CC)  rtprio.c -o rtprio
	 findmsg rtprio.c > rtprio.msg
	 gencat rtprio.cat rtprio.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f rtprio

# copy /usr/bin/rtprio to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/bin/rtprio
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	rtprio
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/bin
	! cp rtprio ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin/rtprio
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
# copy /usr/bin/rtprio to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/bin/rtprio
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/bin
	! cp rtprio ../../../../Filesets.sl/UX-CORE/usr/bin/rtprio
	! chmog 0555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin/rtprio
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
