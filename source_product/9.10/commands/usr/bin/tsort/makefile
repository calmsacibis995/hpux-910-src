default:	tsort.c
	 $(CC)  tsort.c -o tsort
	 findmsg tsort.c > tsort.msg
	 gencat tsort.cat tsort.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f tsort

# copy /usr/bin/tsort to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/tsort
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	tsort
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/bin
	! cp tsort ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin/tsort
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
# copy /usr/bin/tsort to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/tsort
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/bin
	! cp tsort ../../../../Filesets.sl/PROG-AUX/usr/bin/tsort
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin/tsort
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
