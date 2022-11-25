# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/ftio/makefile,v $
# $Revision: 72.6 $

#----------------------------------------------------------------------------#
#									     #
# Ftio Makefile.							     #
#									     #
# David Williams,							     #
# Hewlett-Packard Australian Software Operation.			     #
#----------------------------------------------------------------------------#
#									     #
#  Notes:								     #
#	Building FTIO requires that the BSDmtio.h include file is available  #
#	during the build.  The version of BSDmtio.h can be acquired from the #
#	command 'dump'.  This makefile assumes that the file is located in   #
#	the relative directory of '../dump'.				     #
#

CLDFLAGS=	-Wl,-M8192 
CFLAGS	=	-DNLS 
INCLUDES=	-I../../../etc/dump

SRCS    =       hpux_rel.c main.c fileloader.c filewriter.c tapereader.c\
		tapewriter.c chgreel.c misc.c search.c ftio_mesg.c\
		header.c ipc.c device.c ftio_grep.c

SRC1	=	ftio_rmt.c

OBJS    =       $(SRCS:.c=.o)

OBJ1	=	$(SRC1:.c=.o)

HDRS	=	define.h f_struct.h ftio.h ftio_mesg.h ../../../etc/dump/BSDmtio.h


default: ftio

metrics: ftio.M

ftio:   $(OBJS)
	$(CC) -c -I../../../etc/dump $(CFLAGS) ftio_rmt.c
	$(CC) -I../../../etc/dump $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(OBJ1)

ftio.M:   $(SRCS)
	$(METRICS) -I../../../etc/dump $(CFLAGS) ftio_rmt.c -o $@ $(SRCS) $(SRC1)

clean:
	/bin/rm -f $(OBJS)

clobber: 	clean
	/bin/rm -f ftio a.out ftio.M

# copy /usr/bin/ftio to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/ftio
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	ftio
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp ftio ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin/ftio
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
# copy /usr/bin/ftio to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /usr/bin/ftio
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp ftio ../../../../Filesets.sl/SYS-ADMIN/usr/bin/ftio
	! chmog 0555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin/ftio
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
