default:	atrans.c
	 $(CC)  atrans.c -o atrans
	 findmsg atrans.c > atrans.msg
	 gencat atrans.cat atrans.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f atrans

# copy /bin/atrans to the . relative fileset PROG-AUX
# change mode, owner, and group on /bin/atrans
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	atrans
	mkdir -p ../../../Filesets.sl/PROG-AUX/bin
	! cp atrans ../../../Filesets.sl/PROG-AUX/bin
	! chmog 0555 bin bin ../../../Filesets.sl/PROG-AUX/bin/atrans
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/
# copy /bin/atrans to the . relative fileset PROG-AUX
# change mode, owner, and group on /bin/atrans
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/PROG-AUX/bin
	! cp atrans ../../../Filesets.sl/PROG-AUX/bin/atrans
	! chmog 0555 bin bin ../../../Filesets.sl/PROG-AUX/bin/atrans
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
