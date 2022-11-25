default:	expand.c
	 $(CC)  expand.c -o expand
	 findmsg expand.c > expand.msg
	 gencat expand.cat expand.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f expand

# copy /usr/bin/expand to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/expand
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	expand
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp expand ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/expand
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/expand to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/expand
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp expand ../../../../Filesets.sl/CMDS-AUX/usr/bin/expand
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/expand
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/expand to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/expand
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp expand.cat ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/expand.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/expand.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
