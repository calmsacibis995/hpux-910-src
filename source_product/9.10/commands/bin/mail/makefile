default:	mail.c
	 $(CC)  mail.c -o mail
	 findmsg mail.c > mail.msg
	 gencat mail.cat mail.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f mail

# copy /bin/mail to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/mail
# to 06555, daemon, and mail
# change mode, owner, and group on all the parent
# directories.

install:	mail
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp mail ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 06555 daemon mail ../../../Filesets.sl/CMDS-MIN/bin/mail
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
# copy /bin/mail to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/mail
# to 06555, daemon, and mail
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! cp mail ../../../Filesets.sl/CMDS-MIN/bin/mail
	! chmog 06555 daemon mail ../../../Filesets.sl/CMDS-MIN/bin/mail
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/mail to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/mail
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp mail.cat ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/mail.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/mail.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/mail to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/mail
# to 06555, daemon, and mail
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/bin
	! ln ../../../Filesets.sl/CMDS-MIN/bin/mail ../../../Filesets.sl/CMDS-MIN/bin/rmail
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/mail to the . relative fileset CMDS-MIN
# change mode, owner, and group on /bin/mail
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! ln ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/mail.cat ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/rmail.cat
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
