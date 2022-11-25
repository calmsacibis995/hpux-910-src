default:	eqnchar
	:

# copy /usr/pub/eqnchar to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/pub/eqnchar
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/pub
	! cp eqnchar ../../../../Filesets.sl/CMDS-AUX/usr/pub/eqnchar
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/pub/eqnchar
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/pub
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
