/*
 *
 *  $Header: Makefile,v 1.2.109.1 91/11/21 11:38:09 kcs Exp $
 *  New make file for ARPA-MAN
 *
 */

PRODUCTS=customize copyright

.SOURCE :   $(SRCDIR:/^$/./)

all : build

build : 

debug :

alpha : 

bfa :

install:
	$(INSTALL) FILESET=ARPA-MAN $(PRODUCTS)

rinstall: install
	$(RINSTALL) $(PRODUCTS)

clean :

clobber : clean
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms
