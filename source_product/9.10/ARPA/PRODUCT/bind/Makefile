/*
** $Header: Makefile,v 1.8.109.1 91/11/21 11:39:00 kcs Exp $
*/

all: .FOREGROUND
	(cd res; $(MAKE) all)
	(cd named; $(MAKE) all)
	(cd tools; $(MAKE) all)

build: .FOREGROUND
	(cd res; $(MAKE) build)
	(cd named; $(MAKE) build)
	(cd tools; $(MAKE) build)

clean: .FOREGROUND
	(cd res; $(MAKE) clean)
	(cd named; $(MAKE) clean)
	(cd tools; $(MAKE) clean)

clobber: .FOREGROUND
	(cd res; $(MAKE) clobber)
	(cd named; $(MAKE) clobber)
	(cd named; $(MAKE) clobber)

install: .FOREGROUND
	(cd named; $(MAKE) install)
	(cd tools; $(MAKE) install)
	(cd newconfig; $(MAKE) install)

