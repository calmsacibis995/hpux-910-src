default:	devnm.c
	 $(CC)  devnm.c -o devnm
	 findmsg devnm.c > devnm.msg
	 gencat devnm.cat devnm.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f devnm

# copy /etc/devnm to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/devnm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	devnm
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp devnm ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/devnm
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/devnm to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/devnm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp devnm ../../../Filesets.sl/UX-CORE/etc/devnm
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/devnm
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
