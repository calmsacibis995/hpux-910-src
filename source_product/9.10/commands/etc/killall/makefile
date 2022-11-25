default:	killall.sh
	 /bin/cp killall.sh killall
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f killall.sh

# copy /etc/killall to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/killall
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	killall
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp killall ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/killall
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/killall to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/killall
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp killall ../../../Filesets.sl/UX-CORE/etc/killall
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/killall
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
