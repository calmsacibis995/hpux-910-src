default:	getopt.c
	 $(CC)  getopt.c -o getopt
	 findmsg getopt.c > getopt.msg
	 gencat getopt.cat getopt.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f getopt

# copy /usr/bin/getopt to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/getopt
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	getopt
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp getopt ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/getopt
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/getopt to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/getopt
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp getopt ../../../../Filesets.sl/CMDS-MIN/usr/bin/getopt
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/getopt
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/getopt to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/getopt
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp getopt.cat ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/getopt.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/getopt.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
