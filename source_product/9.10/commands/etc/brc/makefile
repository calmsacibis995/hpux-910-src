default:
	: 

# copy /etc/brc to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/brc
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	brc
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp brc ../../../Filesets.sl/UX-CORE/etc
	! chmog 0544 bin bin ../../../Filesets.sl/UX-CORE/etc/brc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
