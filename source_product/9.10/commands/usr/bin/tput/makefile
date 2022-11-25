default:	tput.c
	$(CC) -Aa -D_HPUX_SOURCE tput.c -o tput -lcurses
	findmsg tput.c > tput.msg
	gencat tput.cat tput.msg

clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f tput

# copy /usr/bin/tput to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/tput
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	tput
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp tput ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/tput
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/tput to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/tput
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp tput ../../../../Filesets.sl/CMDS-AUX/usr/bin/tput
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/tput
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/tput to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/tput
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp tput.cat ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/tput.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/tput.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
