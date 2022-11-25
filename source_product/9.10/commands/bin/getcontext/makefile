default:	getcontext.c
	 $(CC) -Wl,-a,archive  getcontext.c -o getcontext
	 findmsg getcontext.c > getcontext.msg
	 gencat getcontext.cat getcontext.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f getcontext

# copy /bin/getcontext to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/getcontext
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	getcontext
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp getcontext ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/getcontext
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/getcontext to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/getcontext
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp getcontext ../../../Filesets.sl/UX-CORE/bin/getcontext
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/getcontext
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
