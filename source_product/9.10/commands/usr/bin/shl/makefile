# @(#) $Revision: 70.1.1.9 $
# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/shl/makefile,v $
#
# Common makefile for shl (and vt for s300)
#

PTYLIBFILES = ptyrequest.o ptyerrlst.o readline.o

VTLIBFILES =  vtrequest.o setuplan.o getaddress.o getnodenam.o \
	      setndelay.o doio.o vtproto.o setaddress.o vtcommand.o \
	      vtexit.o vtdata.o ftdoio.o hpux_rel.o

default: shlfiles vtfiles

shlfiles: shl

vtfiles: libptyreq.a vt vtserver vtdaemon vtgateway ptydaemon

libptyreq.a: $(PTYLIBFILES)
	@/bin/rm -f $@
	ar qc $@ $(PTYLIBFILES)

libvtreq.a: $(VTLIBFILES)
	@/bin/rm -f $@
	ar qc $@ $(VTLIBFILES)

vt: hpux_rel.o vt.o libptyreq.a libvtreq.a
	cc $(LDFLAGS) -o $@ hpux_rel.o vt.o libptyreq.a libvtreq.a

vtdaemon: hpux_rel.o vtdaemon.o libptyreq.a libvtreq.a
	cc $(LDFLAGS) -o $@ hpux_rel.o vtdaemon.o libptyreq.a libvtreq.a

vtserver: hpux_rel.o vtserver.o libptyreq.a libvtreq.a
	cc $(LDFLAGS) -o $@ hpux_rel.o vtserver.o libptyreq.a libvtreq.a

vtgateway: hpux_rel.o vtgateway.o libptyreq.a libvtreq.a
	cc $(LDFLAGS) -o $@ hpux_rel.o vtgateway.o libptyreq.a libvtreq.a

ptydaemon: hpux_rel.o ptydaemon.o libptyreq.a
	cc $(LDFLAGS) -o $@ hpux_rel.o ptydaemon.o libptyreq.a

shl: shl.o libptyreq.a
	cc $(LDFLAGS) -o $@ shl.o libptyreq.a

clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f libptyreq.a libvtreq.a \
		   vt vtdaemon vtgateway vtserver vtgateway shl ptydaemon

# copy /usr/bin/shl to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/shl
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	shl
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp shl ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/shl
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/shl to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/shl
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/etc
	! cp ptydaemon ../../../../Filesets.sl/CMDS-AUX/etc/ptydaemon
	! chmog 0544 bin bin ../../../../Filesets.sl/CMDS-AUX/etc/ptydaemon
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/etc
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/shl to the . relative fileset SYSCOM
# change mode, owner, and group on /usr/bin/shl
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYSCOM/etc
	! cp vtdaemon ../../../../Filesets.sl/SYSCOM/etc/vtdaemon
	! chmog 0544 bin bin ../../../../Filesets.sl/SYSCOM/etc/vtdaemon
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/etc
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/shl to the . relative fileset SYSCOM
# change mode, owner, and group on /usr/bin/shl
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYSCOM/etc
	! cp vtgateway ../../../../Filesets.sl/SYSCOM/etc/vtgateway
	! chmog 0555 bin bin ../../../../Filesets.sl/SYSCOM/etc/vtgateway
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/etc
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/shl to the . relative fileset SYSCOM
# change mode, owner, and group on /usr/bin/shl
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYSCOM/etc
	! cp vtserver ../../../../Filesets.sl/SYSCOM/etc/vtserver
	! chmog 0555 bin bin ../../../../Filesets.sl/SYSCOM/etc/vtserver
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/etc
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/shl to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/shl
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp shl ../../../../Filesets.sl/CMDS-AUX/usr/bin/shl
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/shl
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/shl to the . relative fileset SYSCOM
# change mode, owner, and group on /usr/bin/shl
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/SYSCOM/usr/bin
	! cp vt ../../../../Filesets.sl/SYSCOM/usr/bin/vt
	! chmog 0555 bin bin ../../../../Filesets.sl/SYSCOM/usr/bin/vt
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/SYSCOM/
	! chmog 555 bin bin ../../../../Filesets.sl/
