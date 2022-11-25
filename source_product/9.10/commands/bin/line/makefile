default:	line.c
	 $(CC)  line.c -o line
	 findmsg line.c > line.msg
	 gencat line.cat line.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f line

# copy /bin/line to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/line
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	line
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp line ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/line
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/line to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/line
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp line ../../../Filesets.sl/UX-CORE/bin/line
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/line
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
