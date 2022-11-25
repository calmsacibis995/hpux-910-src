default:	cc.c
	$(CC) -DPOST80 cc.c -o cc
	$(CC) -DSCC cc.c -o scc
	ln cc c89
	findmsg cc.c > cc.msg
	gencat cc.cat cc.msg
clean:
	rm -f *.o *.msg *.cat

clobber:	clean
	rm -f cc c89 scc
# copy /bin/cc to the . relative fileset BUILDENV
# change mode, owner, and group on /bin/cc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../Filesets.sl/BUILDENV/bin
	! cp c89 ../../../Filesets.sl/BUILDENV/bin/c89
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/bin/c89
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/bin
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/cc to the . relative fileset C
# change mode, owner, and group on /bin/cc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C/bin
	! cp c89 ../../../Filesets.sl/C/bin/c89
	! chmog 0555 bin bin ../../../Filesets.sl/C/bin/c89
	! chmog 555 bin bin ../../../Filesets.sl/C/bin
	! chmog 555 bin bin ../../../Filesets.sl/C/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/cc to the . relative fileset C-MIN
# change mode, owner, and group on /bin/cc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C-MIN/system/C-MIN
	! cp scc ../../../Filesets.sl/C-MIN/system/C-MIN/scc
	! chmog 0555 bin bin ../../../Filesets.sl/C-MIN/system/C-MIN/scc
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/system/C-MIN
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/system/
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
