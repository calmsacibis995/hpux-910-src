/*
 *
 *  $Header: Makefile,v 1.2.109.1 91/11/21 12:02:28 kcs Exp $
 *  New make file for netconfig.serv
 *
 */

PRODUCTS=netbsdsrc

.SOURCE :   $(SRCDIR:/^$/./)

all : build

build : 

debug : .FOREGROUND
	$(MAKE) -e -f $(MAKEFILE) $(-) build \
		CCFLAGS="$(DCFLAGS)" LDFLAGS="$(DLFLAGS)"

alpha : .FOREGROUND
	$(MAKE) -e -f $(MAKEFILE) $(-) build \
		CCFLAGS="$(ACFLAGS)" LDFLAGS="$(ALFLAGS)" \
		LDLIBRARIES="$(ALIBS)"

bfa : .FOREGROUND
	$(MAKE) -e -f $(MAKEFILE) $(-) build \
		CCFLAGS="-B$(BFADB) $(CCFLAGS)" \
		CC="/usr/local/bin/bfacc"

install:
	$(INSTALL) $(PRODUCTS)

rinstall: install
	$(RINSTALL) $(PRODUCTS)

clean :

clobber : clean
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms
