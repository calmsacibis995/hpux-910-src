default:	wall.c
	 $(CC)  wall.c -o wall
	 findmsg wall.c > wall.msg
	 gencat wall.cat wall.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f wall

# copy /etc/wall to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/wall
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	wall
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp wall ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/wall
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/wall to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/wall
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp wall ../../../Filesets.sl/UX-CORE/etc/wall
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/wall
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
