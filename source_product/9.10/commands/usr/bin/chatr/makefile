default:	chatr.c
	 $(CC)  chatr.c -o chatr
	 findmsg chatr.c > chatr.msg
	 gencat chatr.cat chatr.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f chatr

# copy /usr/bin/chatr to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/chatr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	chatr
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp chatr ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/chatr
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/chatr to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/chatr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp chatr ../../../../Filesets.sl/CMDS-AUX/usr/bin/chatr
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/chatr
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
