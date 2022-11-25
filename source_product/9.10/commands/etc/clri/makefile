default:	clri.c
	 $(CC)  clri.c -o clri
	 findmsg clri.c > clri.msg
	 gencat clri.cat clri.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f clri

# copy /etc/clri to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/clri
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	clri
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp clri ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/clri
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/clri to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/clri
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp clri ../../../Filesets.sl/SYS-ADMIN/etc/clri
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/clri
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
