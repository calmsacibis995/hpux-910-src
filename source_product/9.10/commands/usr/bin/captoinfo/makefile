# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/captoinfo/makefile,v $
# $Revision: 64.2.1.4 $

# Common makefile for captoinfo

LIBS = -lcurses
OBJS = capnames.o captoinfo.o cexpand.o iexpand.o infotocap.o otermcap.o print.o


default: captoinfo

captoinfo: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

captoinfo.o: otermcap.h print.h

otermcap.o: otermcap.h

cexpand.o iexpand.o print.o: print.h

clean:
	/bin/rm -f $(OBJS)

clobber: clean
	/bin/rm -f captoinfo

# copy /usr/bin/captoinfo to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/captoinfo
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	captoinfo
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp captoinfo ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin/captoinfo
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
# copy /usr/bin/captoinfo to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/captoinfo
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp captoinfo ../../../../Filesets.sl/SYS-ADMIN/usr/bin/captoinfo
	! chmog 0555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin/captoinfo
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
