default:	fsclean.c
	 $(CC) -Wl,-a,archive  fsclean.c -o fsclean
	 findmsg fsclean.c > fsclean.msg
	 gencat fsclean.cat fsclean.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f fsclean

# copy /etc/fsclean to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/fsclean
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	fsclean
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp fsclean ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/fsclean
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/fsclean to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/fsclean
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp fsclean ../../../Filesets.sl/UX-CORE/etc/fsclean
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/fsclean
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
