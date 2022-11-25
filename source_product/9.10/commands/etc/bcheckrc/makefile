default:
	: 

# copy /etc/bcheckrc to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/bcheckrc
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	bcheckrc
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp bcheckrc ../../../Filesets.sl/UX-CORE/etc
	! chmog 0544 bin bin ../../../Filesets.sl/UX-CORE/etc/bcheckrc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
