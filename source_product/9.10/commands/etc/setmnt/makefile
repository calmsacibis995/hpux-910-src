default:	setmnt.c
	 $(CC)  setmnt.c -o setmnt
	 findmsg setmnt.c > setmnt.msg
	 gencat setmnt.cat setmnt.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f setmnt

# copy /etc/setmnt to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/setmnt
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	setmnt
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp setmnt ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/setmnt
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/setmnt to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/setmnt
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp setmnt ../../../Filesets.sl/UX-CORE/etc/setmnt
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/setmnt
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
