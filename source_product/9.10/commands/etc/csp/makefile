default:	csp.c
	 $(CC) csp.c -o csp -lDUX
	 findmsg csp.c > csp.msg
	 gencat csp.cat csp.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f csp

# copy /etc/csp to the . relative fileset DISKLESS
# change mode, owner, and group on /etc/csp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	csp
	mkdir -p ../../../Filesets.sl/DISKLESS/etc
	! cp csp ../../../Filesets.sl/DISKLESS/etc
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/etc/csp
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/etc
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
# copy /etc/csp to the . relative fileset DISKLESS
# change mode, owner, and group on /etc/csp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/DISKLESS/etc
	! cp csp ../../../Filesets.sl/DISKLESS/etc/csp
	! chmog 0555 bin bin ../../../Filesets.sl/DISKLESS/etc/csp
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/etc
	! chmog 555 bin bin ../../../Filesets.sl/DISKLESS/
	! chmog 555 bin bin ../../../Filesets.sl/
