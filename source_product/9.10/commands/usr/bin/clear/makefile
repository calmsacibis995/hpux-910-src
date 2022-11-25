default:	clear.c
	 $(CC) clear.c -o clear -lcurses
	 findmsg clear.c > clear.msg
	 gencat clear.cat clear.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f clear

# copy /usr/bin/clear to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/clear
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	clear
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp clear ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/clear
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/clear to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/clear
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp clear ../../../../Filesets.sl/CMDS-MIN/usr/bin/clear
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/clear
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
