default:	news.c
	 $(CC)  news.c -o news
	 findmsg news.c > news.msg
	 gencat news.cat news.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f news

# copy /usr/bin/news to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/news
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	news
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp news ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/news
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/news to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/news
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp news ../../../../Filesets.sl/CMDS-AUX/usr/bin/news
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/news
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
