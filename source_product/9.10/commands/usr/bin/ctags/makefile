default:	ctags.c
	 $(CC)  ctags.c -o ctags
	 findmsg ctags.c > ctags.msg
	 gencat ctags.cat ctags.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f ctags

# copy /usr/bin/ctags to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/ctags
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	ctags
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/bin
	! cp ctags ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin/ctags
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
# copy /usr/bin/ctags to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/ctags
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/bin
	! cp ctags ../../../../Filesets.sl/C-TOOLS/usr/bin/ctags
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin/ctags
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
