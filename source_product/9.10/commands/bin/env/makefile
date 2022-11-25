default:	env.c
	 $(CC)  env.c -o env
	 findmsg env.c > env.msg
	 gencat env.cat env.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f env

# copy /bin/env to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/env
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	env
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp env ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/env
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
# copy /bin/env to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/env
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp env ../../../Filesets.sl/CMDS-AUX/bin/env
	! chmog 0555 bin bin ../../../Filesets.sl/CMDS-AUX/bin/env
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/env to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/env
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! cp env.cat ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/env.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C/env.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
