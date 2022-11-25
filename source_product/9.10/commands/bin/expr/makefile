default:	expr.c
	 $(CC)  expr.c -o expr
	 findmsg expr.c > expr.msg
	 gencat expr.cat expr.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f expr

# copy /bin/expr to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/expr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	expr
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp expr ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/expr
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/expr to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/expr
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp expr ../../../Filesets.sl/UX-CORE/bin/expr
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/expr
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/expr to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/expr
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp expr.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/expr.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/expr.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
