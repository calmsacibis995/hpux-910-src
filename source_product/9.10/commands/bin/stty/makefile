default:	stty.c
	 $(CC)  stty.c -o stty
	 findmsg stty.c > stty.msg
	 gencat stty.cat stty.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f stty

# copy /bin/stty to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/stty
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	stty
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp stty ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/stty
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/stty to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/stty
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp stty ../../../Filesets.sl/UX-CORE/bin/stty
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/stty
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
