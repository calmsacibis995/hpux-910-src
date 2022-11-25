default:	chown.c
	 $(CC) chown.c -o chown -lBUILD
	 findmsg chown.c > chown.msg
	 gencat chown.cat chown.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f chown

# copy /bin/chown to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/chown
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	chown
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp chown ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/chown
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/chown to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/chown
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp chown ../../../Filesets.sl/UX-CORE/bin/chown
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/chown
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/chown to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/chown
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp chown.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/chown.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/chown.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
