default:	basename.c
	$(CC) basename.c -o basename
	findmsg basename.c > basename.msg
	gencat basename.cat basename.msg
clean:
	rm -f *.o *.msg *.cat

clobber:	clean
	rm -f basename

# copy /bin/basename to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/basename
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	basename
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp basename ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/basename
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/basename to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/basename
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp basename ../../../Filesets.sl/UX-CORE/bin/basename
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/basename
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
