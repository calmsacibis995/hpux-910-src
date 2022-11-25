# $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/objects/RCS/makefile,v $
# $Revision: 1.3.83.3 $	$Author: root $
# $State: Exp $   	$Locker:  $
# $Date: 93/09/17 16:29:05 $

# @(#) $Revision: 1.3.83.3 $
#
# makefile -- Makefile for creating "analyze"
#

default:
	@if hp9000s300; then				\
	    $(MAKE) analyze.300;			\
	else						\
	    if hp9000s700; then	\
		$(MAKE) analyze.700;			\
	    else					\
		$(MAKE) analyze.800;			\
	    fi;						\
	fi

analyze.300:
	@$(MAKE) SERIES=300 DIR=../../HPUX.300	doit

analyze.700:
	@$(MAKE) SERIES=800 DIR=../../HPUX.800	doit

analyze.800:
	@$(MAKE) SERIES=800 DIR=../../REGION	doit

analyze.300.nodebug:
	@$(MAKE) SERIES=300 DIR=../../HPUX.300.nodebug	doit

analyze.700.nodebug:
	@$(MAKE) SERIES=800 DIR=../../HPUX.800.nodebug	doit

clobber.300:
	@$(MAKE) SERIES=300 clobberit

clobber.700:
	@$(MAKE) SERIES=800 clobberit

clobber.800:
	@$(MAKE) SERIES=800 clobberit

doit:
	@echo "\tGetting IDENT string from kernel sources...";	       \
	here=`/bin/pwd`;					       \
	cd $(DIR);						       \
	id=`make ident`;					       \
	vers=`what < vers.c`;					       \
	vers=`echo $$vers`;					       \
	cd $$here;						       \
	:;							       \
	[ ! -d ../obj.$(SERIES) ] && mkdir ../obj.$(SERIES);	       \
	rm -f version.h;					       \
	ln -s ../obj.$(SERIES)/version.h version.h;		       \
	cd ../obj.$(SERIES);					       \
	:;							       \
	echo "\tlinking analyze sys directory to ../../h";	       \
	/bin/rm -f sys ../h;					       \
	ln -s ../../h sys;					       \
	ln -s ../h ../h;					       \
	:;							       \
	echo "\tcopying sitemap.h and protocol.h into sys...";	       \
	if [ ! -f ../../h.$(SERIES)/sitemap.h ]; then		       \
	    cp ../../dux/sitemap.h ../../h.$(SERIES)/sitemap.h;	       \
	fi;							       \
	if [ ! -f ../../h.$(SERIES)/protocol.h ]; then		       \
	    cp ../../dux/protocol.h ../../h.$(SERIES)/protocol.h;      \
	fi;							       \
	:;							       \
	LDOPTS="-a archive"; export LDOPTS;			       \
	inc="-I../.. -I.";					       \
	echo "\tMaking analyze...";				       \
	$(MAKE) INC="-I../.. -I." IDENT="$$id" ST_VERSION="'$$vers'"   \
	    -f ../objects/Makefile.$(SERIES) analyze.quiet;	       \
	makeRes=$$?;						       \
	if [ $$makeRes -ne 0 ]; then				       \
	    echo "\tError making analyze program!.";		       \
	fi;							       \
	if [ -x analyze ]; then					       \
	    mv -f analyze ../objects/analyze;			       \
	fi;							       \
	exit $$makeRes

clobberit:
	@echo "\tClobbering analyze...\c";	       		       \
	rm -f version.h analyze;				       \
	if [ -d ../obj.$(SERIES) ]; then 			       \
            rm -f ../obj.$(SERIES)/*;				       \
        fi;							       \
	echo " Done."
