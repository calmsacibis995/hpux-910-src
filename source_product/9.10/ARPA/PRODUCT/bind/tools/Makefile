/*
** $Header: Makefile,v 1.12.109.1 91/11/21 11:45:42 kcs Exp $
*/


all: nslookup
	(cd nslookup; $(MAKE) nslookup)

build: 

ccount:
	grep -v "^#" hosts_to_named sig_named | wc -l

clean: 
	$(RM) $(RMFLAGS) *.o *.b core 

clobber: clean
	$(RM) $(RMFLAGS) sig_named hosts_to_named

install: 
	$(INSTALL) sig_named hosts_to_named

rinstall: install
	$(RINSTALL) sig_named hosts_to_named

