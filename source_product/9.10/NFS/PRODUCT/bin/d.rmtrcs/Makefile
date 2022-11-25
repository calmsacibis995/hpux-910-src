# $Source: /source/hpux_source/networking/rcs/nfs90_800/bin/d.rmtrcs/RCS/Makefile,v $
# $Revision: 1.2.109.2 $	$Author: gomathi $
# $State: Exp $   	$Locker:  $
# $Date: 91/11/27 13:34:24 $
#
# Revision 1.2.109.1  91/11/19  14:00:21  14:00:21  kcs (Kernel Control System)
# nfs90 (1.2) NFS Base_for_90
# 
# Revision 1.2  91/11/19  14:00:20  14:00:20  kcs (Kernel Control System)
# Place holder
# 
# Revision 1.2  91/11/01  15:26:51  15:26:51  kcs (Kernel Control System)
# Added the -CND option to Makefile to trigger changes made for CND.
# rev 3.2 author: dds;
# 
# Revision 3.2  87/07/10  16:13:54  16:13:54  dds
# Added the -CND option to Makefile to trigger changes made for CND.
# 
# Revision 1.8  86/08/06  15:09:11  15:09:11  jcm (J. C. Mercier)
# Deleted ACI from options since we do not want
# it anymore.
# 
# Revision 1.7  86/07/23  16:08:19  16:08:19  lam (Susanna Lam)
# Add "-DLANBUG" to CFLAGS.
# 
# Revision 1.6  86/07/22  10:34:35  10:34:35  lam (Susanna Lam)
# Set up CFLAGS to default to "-DHPUX -DKCI -DACI".
# 
# Revision 1.5  86/06/13  09:44:35  09:44:35  jcm (J. C. Mercier)
# Added a comment mentioning KCI as a compilation switch.
# 
# Revision 1.4  86/06/13  09:23:59  jcm (J. C. Mercier)
# Added comments and reorganized compilation switches since
# we now want to have an rmtrcs that knows about ,v files
# on an Indigo rather than on a Vax.
# 
# Revision 1.3  86/06/02  09:54:55  jcm (J. C. Mercier)
# *** empty log message ***
# 
# Revision 1.2  86/06/02  09:48:43  jcm (J. C. Mercier)
# Header
# 
# $Endlog$

# This is the original log for the Makefile.
#
# Revision 1.6  86/02/13  09:46:34  jcm (J. C. Mercier)
# Changed hosts to the hp??? names; trillian got deleted
# and arthur was added.  Also, added CFLAGS to all the
# compilation command lines.  This can be used for turning
# on ACI in the make command line.
# 
# Revision 1.5  86/01/21  11:29:57  jcm (J. C. Mercier)
# Added CFLAGS to rmtrcs so we can make with ACI on.
# 
# Revision 1.4  86/01/21  10:34:30  jcm (J. C. Mercier)
# Changed HPUX to hpux.
# 
# Revision 1.3  86/01/20  16:50:51  gburns (Greg Burns)
# Added entry for hp-ux rmtrcs.  To compile rmtrcs on an Indigo,
# copy everything to a directory on your Indigo, and type 
# "make rmtrcs-hpux".
# 
# Revision 1.2  84/09/04  22:28:18  wallace (Kevin G. Wallace)
# Header added.  
# 

DESTDIR= /mnt/azure/bin
SPHOSTS= hpisoa1 hpisoa2 baruk
VAXHOSTS= hpdsd hpindla
MANL=	/usr/man/manl

# NOTE:
#	The CFLAGS should be typed in as part of the make command line.
#
#	The makefile and source assume that the compilation switches
# 	vax, spectrum and hpux are turned on by default through the host
#	compiler.  (No cross compilation supported here.) You should not
#	have to deal with either one of those three:
#
#	-Dhpux		-	machine running rmtrcs runs HP-UX
#	-Dspectrum	-	machine running rmtrcs is a spectrum machine
#	-Dvax		-	machine running rmtrcs runs 4.2BSD
#
#	You WILL HAVE to type in CFLAGS= and define one of the
#	following two things:
#
#	-DHPUX	-	machine containing ,v files runs HP-UX
#	-DVAX	-	machine containing ,v files runs 4.2BSD
#
#	Optionally, you may want to add:
#
#	-DACI	-	rmtrcs will recognize aci as a command
#	-DKCI	-	rmtrcs will recognize kci as a command
#
CFLAGS=-DHPUX -DKCI -DLANBUG -DCND

all: rmtrcs

rmtrcs: rmtrcs.c
	${CC} ${CFLAGS} -o rmtrcs rmtrcs.c

rmtrcs.1.lpr: rmtrcs.1
	nroff -Tlpr -man rmtrcs.1 > rmtrcs.1.lpr

lint:
	lint rmtrcs.c | grep -v "_flsbuf"

install:
	${CC} {CFLAGS} -O -o ${DESTDIR}/rmtrcs rmtrcs.c

installman:
	cp rmtrcs.1 ${MANL}/rmtrcs.l

remote: install
	for i in ${HOSTS}; do rcp ${DESTDIR}/rmtrcs $$i:${DESTDIR}/rmtrcs; done
	rcp ${DESTDIR}/rmtrcs hptabu:/usr/local/rmtrcs

remoteman: installman
	for i in ${HOSTS} tabu; do rcp rmtrcs.1 $$i:${MANL}/rmtrcs.l; done

clean:
	-rm -f *.o

clobber: clean
	-rm -f rmtrcs
