default:	syslogd.c
	 $(CC)  syslogd.c -o syslogd
	 findmsg syslogd.c > syslogd.msg
	 gencat syslogd.cat syslogd.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f syslogd

# copy /etc/syslogd to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/syslogd
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	syslogd
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp syslogd ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/syslogd
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/syslogd to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/syslogd
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp syslogd ../../../Filesets.sl/UX-CORE/etc/syslogd
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/syslogd
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
