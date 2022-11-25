default:	trace.c
	 $(CC)  trace.c -o trace
	 findmsg trace.c > trace.msg
	 gencat trace.cat trace.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f trace

# copy /usr/bin/trace to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/trace
# to 2555, bin, and sys
# change mode, owner, and group on all the parent
# directories.

install:	trace
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/bin
	! cp trace ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 2555 bin sys ../../../../Filesets.sl/PROG-AUX/usr/bin/trace
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
# copy /usr/bin/trace to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/trace
# to 2555, bin, and sys
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/bin
	! cp trace ../../../../Filesets.sl/PROG-AUX/usr/bin/trace
	! chmog 2555 bin sys ../../../../Filesets.sl/PROG-AUX/usr/bin/trace
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
