default:	link.c
	 $(CC) -Wl,-a,archive  link.c -o link
	 findmsg link.c > link.msg
	 gencat link.cat link.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f link

# copy /etc/link to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/link
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	link
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp link ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/link
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/link to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/link
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp link ../../../Filesets.sl/UX-CORE/etc/link
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/link
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/link to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/link
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! ln ../../../Filesets.sl/UX-CORE/etc/link ../../../Filesets.sl/UX-CORE/etc/unlink
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
