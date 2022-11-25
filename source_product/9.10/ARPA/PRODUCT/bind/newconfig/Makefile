/*
 *
 *  $Header: Makefile,v 1.6.109.2 93/12/15 10:27:05 randyc Exp $
 *
 */

PRODUCTS= boot.cacheonly  boot.primary  boot.secondary  convert_rhosts  \
         db.127.0.0  db.15.19.8  db.cache.arpa  db.cnd  resolv.conf \
	 rfc1034.Z rfc1035.Z nsswitch.conf nssw.dnsfiles nssw.filesdns \
	 nssw.nisfiles
RFCS  =  rfc1034 rfc1035

.SOURCE :   $(SRCDIR:/^$/./)

all : build

build : 

install: compress
	$(INSTALL) $(PRODUCTS)

rinstall: install
	$(RINSTALL) $(PRODUCTS)

clean :

clobber : clean
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

compress:
	for f in $(RFCS) ; do \
		test -f $(SRCDIR:/^$/./)/$f && \
			compress <$(SRCDIR:/^$/./)/$f >$f.Z ; \
	done 
