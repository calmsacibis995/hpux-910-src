default:	strip.c
	 $(CC)  strip.c -o strip
	 findmsg strip.c > strip.msg
	 gencat strip.cat strip.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f strip

# copy /bin/strip to the . relative fileset C-MIN
# change mode, owner, and group on /bin/strip
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	strip
	mkdir -p ../../../Filesets.sl/C-MIN/bin
	! cp strip ../../../Filesets.sl/C-MIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/C-MIN/bin/strip
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/
# copy /bin/strip to the . relative fileset C-MIN
# change mode, owner, and group on /bin/strip
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C-MIN/bin
	! cp strip ../../../Filesets.sl/C-MIN/bin/strip
	! chmog 0555 bin bin ../../../Filesets.sl/C-MIN/bin/strip
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
