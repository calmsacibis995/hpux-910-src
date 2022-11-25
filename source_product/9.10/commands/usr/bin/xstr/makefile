default:	xstr.c
	 $(CC)  xstr.c -o xstr
	 findmsg xstr.c > xstr.msg
	 gencat xstr.cat xstr.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f xstr

# copy /usr/bin/xstr to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/xstr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	xstr
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/bin
	! cp xstr ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin/xstr
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
# copy /usr/bin/xstr to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/xstr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/bin
	! cp xstr ../../../../Filesets.sl/C-TOOLS/usr/bin/xstr
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin/xstr
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
