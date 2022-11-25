default:	syncer.c
	 $(CC)  syncer.c -o syncer
	 findmsg syncer.c > syncer.msg
	 gencat syncer.cat syncer.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f syncer

# copy /etc/syncer to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/syncer
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	syncer
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp syncer ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/syncer
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/syncer to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/syncer
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp syncer ../../../Filesets.sl/UX-CORE/etc/syncer
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/syncer
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
