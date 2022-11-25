default:	cuCDF
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f cuCDF

# copy /etc/newconfig/cuCDF to the . relative fileset SYSCOM
# change mode, owner, and group on /etc/newconfig/cuCDF
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cuCDF
	mkdir -p ../../../../Filesets.sl/SYSCOM/etc/newconfig
	! cp cuCDF ../../../../Filesets.sl/SYSCOM/etc/newconfig
	! chmog 0555 bin bin ../../../../Filesets.sl/SYSCOM/etc/newconfig/cuCDF
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/etc/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/etc/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/
