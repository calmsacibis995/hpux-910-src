default:	od.c
	 $(CC) -Aa -D_HPUX_SOURCE od.c -o od
	 findmsg od.c > od.msg
	 gencat od.cat od.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f od

# copy /bin/od to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/od
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	od
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp od ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/od
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
# copy /bin/od to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/od
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp od ../../../Filesets.sl/CMDS-AUX/bin/od
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/od
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/od to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/od
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp od.cat ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/od.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/od.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/od to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/od
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! ln ../../../Filesets.sl/CMDS-AUX/bin/od ../../../Filesets.sl/CMDS-AUX/bin/xd
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
