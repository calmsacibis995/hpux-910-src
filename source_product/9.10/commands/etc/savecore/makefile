default:	savecore.c
	 $(CC) -g -D_HPUX_SOURCE -Aa savecore.c -o savecore
	 findmsg savecore.c > savecore.msg
	 gencat savecore.cat savecore.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f savecore

# copy /etc/savecore to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/savecore
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	savecore
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp savecore ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/savecore
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/savecore to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/savecore
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp savecore ../../../Filesets.sl/UX-CORE/etc/savecore
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/savecore
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
