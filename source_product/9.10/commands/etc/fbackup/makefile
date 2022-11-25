# $Source: /misc/source_product/9.10/commands.rcs/etc/fbackup/makefile,v $
# $Revision: 72.8 $

#----------------------------------------------------------------------------#
#									     #
#			    Fbackup Makefile.				     #
#									     #
#----------------------------------------------------------------------------#

# CFLAGS  = -DDEBUG -DDEBUG_T -DDEBUG_D
# CFLAGS  = -DDEBUG -DDEBUG_VDI
CCFLAGS = -O
STD_FLAGS = -DCRYPT -DSHARED_LIBS -DSYMLINKS -DHP_NFS -DNFS -DHPNFS -DNFS3_2 -DDISKLESS -DDUX -DCNODE_DEV -DLONGFILENAMES -DLONGFILENAME -DBSDJOBCTL -DQUOTA -DOLD_RFA -DSWFS -DHDBuucp -DNLS -DNLS8 -DNLS16 -DHFS -DCDROM -DPSTAT -D_NAMESPACE_CLEAN -DUT_HOST -DACLS -DAUDIT -DSHADOWPWD -DGETMOUNT -DLOCAL_DISK -DGT_64_FDS -DEUC
CFLAGS = $(CCFLAGS) $(STD_FLAGS) 
LIBS = -lmalloc

GENCAT  = /usr/bin/gencat
FINDMSG = /usr/bin/findmsg -a

MAINSRCS = main.c main2.c main3.c reset.c parse.c search.c inex.c pwgr.c flist.c
RDRSRCS  = reader.c
WRTRSRCS = writer.c writer2.c tape.c rmt.c vdi.c
MSGS     = main.msg rdr.msg wrtr.msg util.msg

#
# Same as *SRCS but with .o extensions
#

MAINOBJS = $(MAINSRCS:.c=.o)
RDROBJS  = $(RDRSRCS:.c=.o)
WRTROBJS = $(WRTRSRCS:.c=.o)


OBJS     = hpux_rel.o util.o $(MAINOBJS) $(RDROBJS) $(WRTROBJS)

EXECUTS  = fbackup fbackuprdr fbackupwrtr
EXECUTS_M =  fbackup.M fbackuprdr.M fbackupwrtr.M

default: $(EXECUTS) fbackup.cat

metrics: $(EXECUTS_M)

fbackup: hpux_rel.o util.o $(MAINOBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o fbackup hpux_rel.o util.o $(MAINOBJS) $(LIBS)

fbackup.M: hpux_rel.c util.c $(MAINSRCS)
	$(METRICS) $(CFLAGS) -o fbackup.M hpux_rel.c util.c $(MAINSRCS) $(LIBS)

	
#				NOTE: util.o is not needed for fbackuprdr
fbackuprdr: hpux_rel.o $(RDROBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o fbackuprdr hpux_rel.o $(RDROBJS) $(LIBS)

fbackuprdr.M: hpux_rel.c $(RDRSRCS)
	$(METRICS) $(CFLAGS) -o fbackuprdr.M hpux_rel.c $(RDRSRCS) $(LIBS)

fbackupwrtr: hpux_rel.o util.o $(WRTROBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o fbackupwrtr hpux_rel.o util.o $(WRTROBJS) $(LIBS)

fbackupwrtr.M: hpux_rel.c util.c $(WRTRSRCS)
	$(METRICS) $(CFLAGS) -o fbackupwrtr.M hpux_rel.c util.c $(WRTRSRCS) $(LIBS)

rmt.o: rmt.c
	$(CC) $(CFLAGS) $(INCLUDES) -c rmt.c $(LIBS)

$(OBJS): head.h 

clean: 
	/bin/rm -f $(OBJS) $(MSGS)

clobber: clean
	/bin/rm -f $(EXECUTS) $(EXECUTS_M) fbackup.cat

#
# Targets for creating the fbackup.cat file and necessary .msg files
#

fbackup.cat: $(MSGS)
	$(GENCAT) fbackup.cat main.msg rdr.msg wrtr.msg util.msg

main.msg: $(MAINSRCS)
	$(FINDMSG) $(MAINSRCS) > main.msg

rdr.msg:  $(RDRSRCS)
	$(FINDMSG) $(RDRSRCS) > rdr.msg

wrtr.msg: $(WRTRSRCS)
	$(FINDMSG) $(WRTRSRCS) > wrtr.msg

util.msg:  util.c
	$(FINDMSG) util.c > util.msg

# copy /etc/fbackup to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/fbackup
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	fbackup
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp fbackup ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/fbackup
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/fbackup to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/fbackup
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp fbackup ../../../Filesets.sl/SYS-ADMIN/etc/fbackup
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/fbackup
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/fbackup to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/fbackup
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp fbackuprdr ../../../Filesets.sl/SYS-ADMIN/etc/fbackuprdr
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/fbackuprdr
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/fbackup to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/fbackup
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp fbackupwrtr ../../../Filesets.sl/SYS-ADMIN/etc/fbackupwrtr
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/fbackupwrtr
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/fbackup to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/fbackup
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/usr/lib/nls/C
	! cp fbackup.cat ../../../Filesets.sl/SYS-ADMIN/usr/lib/nls/C/fbackup.cat
	! chmog 0444 bin bin ../../../Filesets.sl/SYS-ADMIN/usr/lib/nls/C/fbackup.cat
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
