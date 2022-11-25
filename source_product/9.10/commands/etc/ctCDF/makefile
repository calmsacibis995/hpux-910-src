default:	ctCDF
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f ctCDF

# copy /etc/newconfig/ctCDF to the . relative fileset SYSCOM
# change mode, owner, and group on /etc/newconfig/ctCDF
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	ctCDF
	mkdir -p ../../../../Filesets.sl/SYSCOM/etc/newconfig
	! cp ctCDF ../../../../Filesets.sl/SYSCOM/etc/newconfig
	! chmog 0555 bin bin ../../../../Filesets.sl/SYSCOM/etc/newconfig/ctCDF
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/etc/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/etc/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/
