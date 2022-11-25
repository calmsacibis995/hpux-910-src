default:	from.c
	 $(CC)  from.c -o from
	 findmsg from.c > from.msg
	 gencat from.cat from.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f from

# copy /usr/bin/from to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/from
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	from
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/bin
	! cp from ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin/from
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
# copy /usr/bin/from to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/from
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/bin
	! cp from ../../../../Filesets.sl/MAILERS/usr/bin/from
	! chmog 0555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin/from
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
