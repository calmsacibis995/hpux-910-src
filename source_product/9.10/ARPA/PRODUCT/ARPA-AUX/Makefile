# $Source: /source/hpux_source/networking/rcs/arpa90_800/ARPA-AUX/RCS/Makefile,v $
# $Revision: 1.2.109.1 $	$Author: gomathi $
# $State: Exp $   	$Locker:  $
# $Date: 92/04/28 16:10:43 $

/*
 *
 *  New make file for ARPA-AUX
 *
 */

PRODUCTS=customize copyright decustomize

.SOURCE :   $(SRCDIR:/^$/./)

all : build

build :

debug :

alpha :

bfa :

install:
        $(INSTALL) -f ARPA-AUX $(PRODUCTS)

rinstall: install
        $(RINSTALL) -f ARPA-AUX $(PRODUCTS)

clean :

clobber : clean
        $(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms
