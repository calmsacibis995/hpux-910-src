default:	mkstr.c
	 $(CC)  mkstr.c -o mkstr
	 findmsg mkstr.c > mkstr.msg
	 gencat mkstr.cat mkstr.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f mkstr

# copy /usr/bin/mkstr to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/mkstr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	mkstr
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/bin
	! cp mkstr ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin/mkstr
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
# copy /usr/bin/mkstr to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/mkstr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/bin
	! cp mkstr ../../../../Filesets.sl/PROG-AUX/usr/bin/mkstr
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin/mkstr
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
