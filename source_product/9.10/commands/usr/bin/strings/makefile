default:	strings.c
	 $(CC)  strings.c -o strings
	 findmsg strings.c > strings.msg
	 gencat strings.cat strings.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f strings

# copy /usr/bin/strings to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/strings
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	strings
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp strings ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/strings
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/strings to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/strings
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp strings ../../../../Filesets.sl/CMDS-AUX/usr/bin/strings
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/strings
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/strings to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/strings
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp strings.cat ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/strings.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/strings.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
