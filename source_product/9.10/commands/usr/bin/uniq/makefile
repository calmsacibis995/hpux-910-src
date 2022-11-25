default:	uniq.c
	 $(CC)  uniq.c -o uniq
	 findmsg uniq.c > uniq.msg
	 gencat uniq.cat uniq.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f uniq

# copy /usr/bin/uniq to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/uniq
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	uniq
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp uniq ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/uniq
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/uniq to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/uniq
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp uniq ../../../../Filesets.sl/CMDS-AUX/usr/bin/uniq
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/uniq
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/uniq to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/uniq
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp uniq.cat ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/uniq.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/uniq.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
