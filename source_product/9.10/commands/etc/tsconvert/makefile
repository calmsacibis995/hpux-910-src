default:	tsconvert.c
	 $(CC)  tsconvert.c -o tsconvert
	 findmsg tsconvert.c > tsconvert.msg
	 gencat tsconvert.cat tsconvert.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f tsconvert

# copy /etc/tsconvert to the . relative fileset AUDIT
# change mode, owner, and group on /etc/tsconvert
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	tsconvert
	mkdir -p ../../../Filesets.sl/AUDIT/etc
	! cp tsconvert ../../../Filesets.sl/AUDIT/etc
	! chmog 0555 bin bin ../../../Filesets.sl/AUDIT/etc/tsconvert
	! chmog 555 bin bin ../../../Filesets.sl/AUDIT/etc
	! chmog 555 bin bin ../../../Filesets.sl/AUDIT/
# copy /etc/tsconvert to the . relative fileset AUDIT
# change mode, owner, and group on /etc/tsconvert
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/AUDIT/etc
	! cp tsconvert ../../../Filesets.sl/AUDIT/etc/tsconvert
	! chmog 0555 bin bin ../../../Filesets.sl/AUDIT/etc/tsconvert
	! chmog 555 bin bin ../../../Filesets.sl/AUDIT/etc
	! chmog 555 bin bin ../../../Filesets.sl/AUDIT/
	! chmog 555 bin bin ../../../Filesets.sl/
