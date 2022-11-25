default:	join.c
	 $(CC)  join.c -o join
	 findmsg join.c > join.msg
	 gencat join.cat join.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f join

# copy /usr/bin/join to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/join
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	join
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp join ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/join
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/join to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/join
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp join ../../../../Filesets.sl/CMDS-AUX/usr/bin/join
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/join
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
