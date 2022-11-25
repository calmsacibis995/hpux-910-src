/*
  The convert files are for converting pre-9.00 gated config files to
  Gated 2.1 format available in 9.00.
*/
CONVERTDIR =	./convert
CONVERTFILES =	README.conv conv_config

HPDIR  =	..		/* location of Copyright from Cornell */
HPFILES	=	README gated.conf Copyright

HPDIR1  =	..		/* location of Copyright from Cornell */
HPFILE1	=	aux/README 

AUXDIR	=	../aux		/* location of aux file from Cornell */
AUXFILES =	changes.awk gated-mon gw-mon gw-mon.log \
		redirects.awk rotatelogs

CONFDIR = 	../conf		/* location of conf files from Cornell */

/*
	Cornell's names for the example config files to be delivered 
*/
CONFFILES =	gated.conf.aarnet gated.conf.anl gated.conf.bgp-simple \
		gated.conf.concert-net gated.conf.egp-simple1 \
		gated.conf.egp-simple2 gated.conf.rip-simple

/*
	Actual shortened names for the example config files to be delivered
*/
SHORTCFILES =	aarnet anl bgp-simple concert-net egp-simple1 \
		egp-simple2 rip-simple


all	:	build

build :		aux conf hpdir convert

clobber:	/* removes copies of files, not the real ones */
	/bin/rm -rf $(SRCDIR:/^$/./)/aux
	/bin/rm -rf $(SRCDIR:/^$/./)/conf
	/bin/rm -rf $(SRCDIR:/^$/./)/hpdir/Copyright
	cd $(SRCDIR:/^$/./)/convert
	make clobber
	cd $(SRCDIR:/^$/./)

install:
        SRCDIR=$(SRCDIR:/^$/./)/aux $(INSTALL) $(AUXFILES)

        SRCDIR=$(SRCDIR:/^$/./)/conf $(INSTALL) $(SHORTCFILES)

        SRCDIR=$(SRCDIR:/^$/./)/hpdir $(INSTALL) $(HPFILES)

	SRCDIR=$(SRCDIR:/^$/./)/hpdir/aux $(INSTALL) $(HPFILE1)	

        SRCDIR=$(SRCDIR:/^$/./)/convert $(INSTALL) $(CONVERTFILES)

aux:
    /bin/mkdir -p $(SRCDIR:/^$/./)/aux
    /bin/cp $(AUXFILES:D=$(SRCDIR)/$(AUXDIR)/:P:B:S:) $(SRCDIR:/^$/./)/aux

conf:
    /bin/mkdir -p $(SRCDIR:/^$/./)/conf
    /bin/cp $(CONFFILES:D=$(SRCDIR)/$(CONFDIR)/:P:B:S:) $(SRCDIR:/^$/./)/conf
    $(SRCDIR:/^$/./)/shorten.sh $(SRCDIR:/^$/./)/conf $(CONFFILES)

hpdir:
    cd  $(SRCDIR:/^$/./)/
    /bin/cp $(SRCDIR:/^$/./)/$(HPDIR)/Copyright $(SRCDIR:/^$/./)/hpdir

convert:
    cd $(SRCDIR:/^$/./)/convert
    make build
    cd $(SRCDIR:/^$/./)
