default:	ar.ranlib.c
	 $(CC)  ar.ranlib.c -o ar.ranlib
	 findmsg ar.ranlib.c > ar.ranlib.msg
	 gencat ar.ranlib.cat ar.ranlib.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f ar.ranlib

# copy /usr/lib/ar.ranlib to the . relative fileset C-MIN
# change mode, owner, and group on /usr/lib/ar.ranlib
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	ar.ranlib
	mkdir -p ../../../../Filesets.sl/C-MIN/usr/lib
	! cp ar.ranlib ../../../../Filesets.sl/C-MIN/usr/lib
	! chmog 0555 bin bin ../../../../Filesets.sl/C-MIN/usr/lib/ar.ranlib
	! chmog 555 bin bin ../../../../Filesets.sl/C-MIN/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-MIN/
# copy /usr/lib/ar.ranlib to the . relative fileset C-MIN
# change mode, owner, and group on /usr/lib/ar.ranlib
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-MIN/usr/lib
	! cp ar.ranlib ../../../../Filesets.sl/C-MIN/usr/lib/ar.ranlib
	! chmog 0555 bin bin ../../../../Filesets.sl/C-MIN/usr/lib/ar.ranlib
	! chmog 555 bin bin ../../../../Filesets.sl/C-MIN/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
