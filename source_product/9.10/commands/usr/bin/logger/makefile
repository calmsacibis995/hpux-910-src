default:	logger.c
	 $(CC)  logger.c -o logger
	 findmsg logger.c > logger.msg
	 gencat logger.cat logger.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f logger

# copy /usr/bin/logger to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/logger
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	logger
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp logger ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin/logger
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
# copy /usr/bin/logger to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/logger
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp logger ../../../../Filesets.sl/SYS-ADMIN/usr/bin/logger
	! chmog 0555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin/logger
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/logger to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/logger
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/lib/nls/C
	! cp logger.cat ../../../../Filesets.sl/SYS-ADMIN/usr/lib/nls/C/logger.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/lib/nls/C/logger.cat
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
