default:	astrn.c
	 $(CC)  astrn.c -o astrn
	 findmsg astrn.c > astrn.msg
	 gencat astrn.cat astrn.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f astrn

# copy /bin/astrn to the . relative fileset PROG-AUX
# change mode, owner, and group on /bin/astrn
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	astrn
	mkdir -p ../../../Filesets.sl/PROG-AUX/bin
	! cp astrn ../../../Filesets.sl/PROG-AUX/bin
	! chmog 0555 bin bin ../../../Filesets.sl/PROG-AUX/bin/astrn
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/
# copy /bin/astrn to the . relative fileset PROG-AUX
# change mode, owner, and group on /bin/astrn
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/PROG-AUX/bin
	! cp astrn ../../../Filesets.sl/PROG-AUX/bin/astrn
	! chmog 0555 bin bin ../../../Filesets.sl/PROG-AUX/bin/astrn
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/bin
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
