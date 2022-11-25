default:
	:

# copy /usr/lib/charmaps to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/lib/charmaps
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/lib/charmaps
	! cp ascii.cm ../../../../Filesets.sl/NLS-CORE/usr/lib/charmaps/ascii.cm
	! chmog 0444 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/charmaps/ascii.cm
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/charmaps
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
