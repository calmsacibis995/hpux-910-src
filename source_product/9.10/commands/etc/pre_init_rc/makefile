default:
	: 

# copy /etc/pre_init_rc to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/pre_init_rc
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	pre_init_rc
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp pre_init_rc ../../../Filesets.sl/UX-CORE/etc
	! chmog 0544 bin bin ../../../Filesets.sl/UX-CORE/etc/pre_init_rc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
