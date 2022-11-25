default:	newgrp.c
	 $(CC)  newgrp.c -o newgrp
	 findmsg newgrp.c > newgrp.msg
	 gencat newgrp.cat newgrp.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f newgrp

# copy /bin/newgrp to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/newgrp
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.

install:	newgrp
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp newgrp ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 04555 root bin ../../../Filesets.sl/CMDS-AUX/bin/newgrp
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
# copy /bin/newgrp to the . relative fileset CMDS-AUX
# change mode, owner, and group on /bin/newgrp
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-AUX/bin
	! cp newgrp ../../../Filesets.sl/CMDS-AUX/bin/newgrp
	! chmog 04555 root bin ../../../Filesets.sl/CMDS-AUX/bin/newgrp
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
