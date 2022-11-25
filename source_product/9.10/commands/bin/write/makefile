default:	write.c
	 $(CC)  write.c -o write
	 findmsg write.c > write.msg
	 gencat write.cat write.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f write

# copy /bin/write to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/write
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	write
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp write ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/write
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
# copy /bin/write to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/write
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp write ../../../Filesets.sl/CMDS-AUX/bin/write
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/write
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/write to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/write
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp write.cat ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/write.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/write.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
