default:	swapon.c
	 $(CC)  swapon.c -o swapon
	 findmsg swapon.c > swapon.msg
	 gencat swapon.cat swapon.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f swapon

# copy /etc/swapon to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/swapon
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	swapon
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp swapon ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/swapon
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/swapon to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/swapon
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp swapon ../../../Filesets.sl/UX-CORE/etc/swapon
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/swapon
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
