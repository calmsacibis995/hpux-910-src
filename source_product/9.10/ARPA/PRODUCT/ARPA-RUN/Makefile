/*
 *
 *  $Header: Makefile,v 1.4.109.1 91/11/21 11:38:25 kcs Exp $
 *  New make file for ARPA-RUN
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
	$(INSTALL) -f ARPA-RUN $(PRODUCTS)

rinstall: install
	$(RINSTALL) -f ARPA-RUN $(PRODUCTS)

clean :

clobber : clean
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms
