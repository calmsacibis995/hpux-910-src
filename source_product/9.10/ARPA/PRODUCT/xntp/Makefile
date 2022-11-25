/* 
* $Header: Makefile,v 1.2.109.3 95/01/24 16:16:55 mike Exp $
* 
* Makefile for xntp
*/

/* This version of the Makefile is intended for every architecture/release
   EXCEPT s300/400 9.1 and succeeding releases, if any.
*/

SHELL	= /bin/sh

# ifdef Debug
SUBMAKEOPTS	= -k
CCFLAGS		= -g $(NEWCCFLAGS)
LDFLAGS		= $(NEWLDFLAGS)
# else
SUBMAKEOPTS	= -k
CCFLAGS		= -O -e $(NEWCCFLAGS)
LDFLAGS		= -s $(NEWLDFLAGS)
# endif


PRODUCT	=  lib adjtime authstuff ntpdate ntpq ntptrace xntpd xntpdc docs

all prod trace debug clean clobber pfa: .FOREGROUND
	# set -x
	for dir in $(PRODUCT)
	do
		# echo "$(<)"
		# the following rm ensures that we're using the latest Makefile
	    	test -d $dir && \
		  ( cd $dir; rm -f *.mo *.ms; $(MAKE) $(-) $(SUBMAKEOPTS) $(<) )
	done

# Not correct yet
install: .FORCE
	test "${INS}" && ${INS} ${EXTRA}
	test "${INS}" && \
	    for dir in $(PRODUCT) do \
		test -d $${dir} && \
		  ( cd $dir; rm -f *.mo *.ms; $(MAKE) $(-) $(SUBMAKEOPTS) $(<) ) \
	    done
	test "${INS}" && ${INS} ${INSLINKS}

.FORCE:
	@rm -rf .FORCE
