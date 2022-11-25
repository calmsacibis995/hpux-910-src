default:	cu.c
	 $(CC)  cu.c -o cu
	 findmsg cu.c > cu.msg
	 gencat cu.cat cu.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f cu

# copy /usr/bin/cu to the . relative fileset SYSCOM
# change mode, owner, and group on /usr/bin/cu
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cu
	mkdir -p ../../../../Filesets.sl/SYSCOM/usr/bin
	! cp cu ../../../../Filesets.sl/SYSCOM/usr/bin
	! chmog 04555 root bin ../../../../Filesets.sl/SYSCOM/usr/bin/cu
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/
# copy /usr/bin/cu to the . relative fileset SYSCOM
# change mode, owner, and group on /usr/bin/cu
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYSCOM/usr/bin
	! cp cu ../../../../Filesets.sl/SYSCOM/usr/bin/cu
	! chmog 04555 root bin ../../../../Filesets.sl/SYSCOM/usr/bin/cu
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/cu to the . relative fileset SYSCOM
# change mode, owner, and group on /usr/bin/cu
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYSCOM/usr/lib/nls/C
	! cp cu.cat ../../../../Filesets.sl/SYSCOM/usr/lib/nls/C/cu.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/SYSCOM/usr/lib/nls/C/cu.cat
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/
	! chmog 555 bin bin ../../../../Filesets.sl/
