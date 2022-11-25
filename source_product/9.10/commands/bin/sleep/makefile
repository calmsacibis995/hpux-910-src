default:	sleep.c
	 $(CC)  sleep.c -o sleep
	 findmsg sleep.c > sleep.msg
	 gencat sleep.cat sleep.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f sleep

# copy /bin/sleep to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sleep
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	sleep
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp sleep ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/sleep
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/sleep to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sleep
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp sleep ../../../Filesets.sl/UX-CORE/bin/sleep
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/sleep
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/sleep to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sleep
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp sleep.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/sleep.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/sleep.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
