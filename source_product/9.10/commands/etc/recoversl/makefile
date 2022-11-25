default:	recoversl.c
	 $(CC) -Wl,-a,archive  recoversl.c -o recoversl
	 findmsg recoversl.c > recoversl.msg
	 gencat recoversl.cat recoversl.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f recoversl

# copy /etc/recoversl to the . relative fileset CORE-SHLIBS
# change mode, owner, and group on /etc/recoversl
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	recoversl
	mkdir -p ../../../Filesets.sl/CORE-SHLIBS/etc
	! cp recoversl ../../../Filesets.sl/CORE-SHLIBS/etc
	! chmog 0555 bin bin ../../../Filesets.sl/CORE-SHLIBS/etc/recoversl
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/etc
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/
# copy /etc/recoversl to the . relative fileset CORE-SHLIBS
# change mode, owner, and group on /etc/recoversl
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CORE-SHLIBS/etc
	! cp recoversl ../../../Filesets.sl/CORE-SHLIBS/etc/recoversl
	! chmog 0555 bin bin ../../../Filesets.sl/CORE-SHLIBS/etc/recoversl
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/etc
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/recoversl to the . relative fileset CORE-SHLIBS
# change mode, owner, and group on /etc/recoversl
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CORE-SHLIBS/usr/lib/nls/C
	! cp recoversl.cat ../../../Filesets.sl/CORE-SHLIBS/usr/lib/nls/C/recoversl.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CORE-SHLIBS/usr/lib/nls/C/recoversl.cat
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/
	! chmog 555 bin bin ../../../Filesets.sl/
