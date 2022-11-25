default:	dirname.c
	 $(CC)  dirname.c -o dirname
	 findmsg dirname.c > dirname.msg
	 gencat dirname.cat dirname.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f dirname

# copy /bin/dirname to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/dirname
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	dirname
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp dirname ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/dirname
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/dirname to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/dirname
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp dirname ../../../Filesets.sl/UX-CORE/bin/dirname
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/dirname
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
