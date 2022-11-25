default:	touch.c
	 $(CC)  touch.c -o touch
	 findmsg touch.c > touch.msg
	 gencat touch.cat touch.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f touch

# copy /bin/touch to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/touch
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	touch
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp touch ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/touch
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/touch to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/touch
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp touch ../../../Filesets.sl/UX-CORE/bin/touch
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/touch
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
