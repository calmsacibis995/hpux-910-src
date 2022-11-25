default:	echo.c
	 $(CC)  echo.c -o echo
	 findmsg echo.c > echo.msg
	 gencat echo.cat echo.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f echo

# copy /bin/echo to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/echo
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	echo
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp echo ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/echo
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/echo to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/echo
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp echo ../../../Filesets.sl/UX-CORE/bin/echo
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/echo
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
