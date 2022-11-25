default:	chgrp.c
	 $(CC) chgrp.c -o chgrp -lBUILD
	 findmsg chgrp.c > chgrp.msg
	 gencat chgrp.cat chgrp.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f chgrp

# copy /bin/chgrp to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/chgrp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	chgrp
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp chgrp ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/chgrp
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/chgrp to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/chgrp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp chgrp ../../../Filesets.sl/UX-CORE/bin/chgrp
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/chgrp
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/chgrp to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/chgrp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp chgrp.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/chgrp.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/chgrp.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
