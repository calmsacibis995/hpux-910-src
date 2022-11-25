default:	nohup.c
	 $(CC)  nohup.c -o nohup
	 findmsg nohup.c > nohup.msg
	 gencat nohup.cat nohup.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f nohup

# copy /bin/nohup to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/nohup
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	nohup
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp nohup ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/nohup
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /bin/nohup to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/nohup
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp nohup ../../../Filesets.sl/CMDS-MIN/bin/nohup
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/nohup
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/nohup to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/nohup
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp nohup.cat ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/nohup.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/nohup.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
