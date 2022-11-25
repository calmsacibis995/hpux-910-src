default:	ar.c
	$(CC) ar.c -o ar
	findmsg ar.c > ar.msg
	gencat ar.cat ar.msg
clean:
	rm -f *.o *.msg *.cat

clobber:	clean
	rm -f ar

# copy /bin/ar to the . relative fileset C-MIN
# change mode, owner, and group on /bin/ar
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	ar
	mkdir -p ../../../Filesets.sl/C-MIN/bin
	! cp ar ../../../Filesets.sl/C-MIN/bin
	! chmog 0555 bin bin ../../../Filesets.sl/C-MIN/bin/ar
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/
# copy /bin/ar to the . relative fileset C-MIN
# change mode, owner, and group on /bin/ar
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C-MIN/bin
	! cp ar ../../../Filesets.sl/C-MIN/bin/ar
	! chmog 0555 bin bin ../../../Filesets.sl/C-MIN/bin/ar
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/bin
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/ar to the . relative fileset C-MIN
# change mode, owner, and group on /bin/ar
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C-MIN/usr/lib/nls/C
	! cp ar.cat ../../../Filesets.sl/C-MIN/usr/lib/nls/C/ar.cat
	! chmog 0444 bin bin ../../../Filesets.sl/C-MIN/usr/lib/nls/C/ar.cat
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
