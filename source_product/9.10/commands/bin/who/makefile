default:	who.c
	 $(CC)  who.c -o who
	 findmsg who.c > who.msg
	 gencat who.cat who.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f who

# copy /bin/who to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/who
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	who
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp who ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/who
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /bin/who to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/who
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp who ../../../Filesets.sl/CMDS-MIN/bin/who
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-MIN/bin/who
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/who to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/who
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp who.cat ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/who.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/who.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
