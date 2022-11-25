/*
 *
 *  $Header: Makefile,v 1.3.109.1 91/11/21 11:37:34 kcs Exp $
 *  New make file for ARPA-INC
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
	$(INSTALL) -f ARPA-INC $(PRODUCTS)

rinstall: install
	$(RINSTALL) -f ARPA-INC $(PRODUCTS)

clean :

clobber : clean
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms
