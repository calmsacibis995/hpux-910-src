default:	printf.c
	 $(CC)  printf.c -o printf
	 findmsg printf.c > printf.msg
	 gencat printf.cat printf.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f printf

# copy /usr/bin/printf to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/printf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	printf
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp printf ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/printf
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/printf to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/printf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp printf ../../../../Filesets.sl/CMDS-AUX/usr/bin/printf
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/printf
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
