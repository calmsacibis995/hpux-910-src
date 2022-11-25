default:	script.c
	 $(CC)  script.c -o script
	 findmsg script.c > script.msg
	 gencat script.cat script.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f script

# copy /usr/bin/script to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/script
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	script
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp script ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/script
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/script to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/script
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp script ../../../../Filesets.sl/CMDS-AUX/usr/bin/script
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/script
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
