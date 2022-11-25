default:	wc.c
	 $(CC)  wc.c -o wc
	 findmsg wc.c > wc.msg
	 gencat wc.cat wc.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f wc

# copy /bin/wc to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/wc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	wc
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp wc ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/wc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/wc to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/wc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp wc ../../../Filesets.sl/UX-CORE/bin/wc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/wc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/wc to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/wc
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp wc.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/wc.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/wc.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
