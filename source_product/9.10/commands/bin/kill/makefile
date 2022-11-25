default:	kill.c
	 $(CC)  kill.c -o kill
	 findmsg kill.c > kill.msg
	 gencat kill.cat kill.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f kill

# copy /bin/kill to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/kill
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	kill
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp kill ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/kill
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/kill to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/kill
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp kill ../../../Filesets.sl/UX-CORE/bin/kill
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/kill
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
