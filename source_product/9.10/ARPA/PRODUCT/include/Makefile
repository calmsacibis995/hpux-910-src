/*
 *
 *  $Header: Makefile,v 1.3.109.1 91/11/21 11:59:44 kcs Exp $
 *  New make file for include
 *
 */

INCLUDES  = netdb.h resolv.h
AINCLUDES = nameser.h inet.h ftp.h telnet.h tftp.h
PINCLUDES = rwhod.h

all : build

build : 

debug :

alpha :

bfa :

install:
	$(INSTALL) $(INCLUDES)
	SRCDIR=$(SRCDIR:/^$/./)/arpa $(INSTALL) $(AINCLUDES)
	SRCDIR=$(SRCDIR:/^$/./)/protocols $(INSTALL) $(PINCLUDES)

rinstall:
	$(RINSTALL) $(INCLUDES)
	SRCDIR=$(SRCDIR:/^$/./)/arpa $(RINSTALL) $(AINCLUDES)
	SRCDIR=$(SRCDIR:/^$/./)/protocols $(RINSTALL) $(PINCLUDES)

clean:

clobber:
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./) ; ccount -p -I. *.h
	cd $(SRCDIR:/^$/./)/arpa ;  ccount -p -I..  *.h
	cd $(SRCDIR:/^$/./)/protocols ; ccount -p -I.. *.h
