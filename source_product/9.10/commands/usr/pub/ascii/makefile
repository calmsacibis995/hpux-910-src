default:	ascii
	:

# copy /usr/pub/ascii to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/pub/ascii
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/pub
	! cp ascii ../../../../Filesets.sl/CMDS-AUX/usr/pub/ascii
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/pub/ascii
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/pub
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
