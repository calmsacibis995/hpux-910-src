default:	ioscan.c ioscan.h hpux_rel.c
	$(CC) ioscan.c hpux_rel.c -o ioscan -lio

# copy /etc/ioscan_WSIO to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/ioscan_WSIO
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp ioscan ../../../Filesets.sl/UX-CORE/etc/ioscan
	! chmog 04555 root bin ../../../Filesets.sl/UX-CORE/etc/ioscan
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
