default:
	: 

# copy /etc/set_parms to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/set_parms
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	set_parms
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp set_parms ../../../Filesets.sl/UX-CORE/etc
	! chmog 0544 bin bin ../../../Filesets.sl/UX-CORE/etc/set_parms
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
