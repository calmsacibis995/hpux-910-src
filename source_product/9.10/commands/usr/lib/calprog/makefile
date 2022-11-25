default:	calprog.c
	 $(CC)  calprog.c -o calprog
	 findmsg calprog.c > calprog.msg
	 gencat calprog.cat calprog.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f calprog

# copy /usr/lib/calprog to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/lib/calprog
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	calprog
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! cp calprog ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/calprog
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/lib/calprog to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/lib/calprog
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! cp calprog ../../../../Filesets.sl/CMDS-AUX/usr/lib/calprog
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/calprog
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
