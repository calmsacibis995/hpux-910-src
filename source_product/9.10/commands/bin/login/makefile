default:	login.c
	 $(CC) -Wl,-a,archive -DRFLAG login.c -o login
	 findmsg login.c > login.msg
	 gencat login.cat login.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f login

# copy /bin/login to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/login
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.

install:	login
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp login ../../../Filesets.sl/UX-CORE/bin
	! chmog 04555 root bin ../../../Filesets.sl/UX-CORE/bin/login
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/login to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/login
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp login ../../../Filesets.sl/UX-CORE/bin/login
	! chmog 04555 root bin ../../../Filesets.sl/UX-CORE/bin/login
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
