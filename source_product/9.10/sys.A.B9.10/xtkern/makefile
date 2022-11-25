
#
#	Makefile for building the kernel used by the Xterminal.
#
#	NOTE: making locore.o is special cased since locore.o goes in
#	      two libraries. Bummer...
#


XTKERN_DEST = ../Filesets/XTERM300/usr/lib
SHELL       = /bin/ksh
XTBLD       = ../XTKERN.300.nodebug

all:	get_bits xtkern.300
	@echo Installing xtkern.300
	@if [ ! -d $(XTKERN_DEST) ] ; then mkdir -p $(XTKERN_DEST) ; fi
	@chown -R bin:bin ../Filesets/XTERM300
	@chmod -R 0555 ../Filesets/XTERM300
	@cp $(XTBLD)/hp-ux $(XTKERN_DEST)/xtkern.300
	@chmod 0444 $(XTKERN_DEST)/xtkern.300
	@chown bin:bin $(XTKERN_DEST)/xtkern.300
	@echo Done.


#
#	Get the latest non-debug bits.
#
get_bits:;
	@if [ ! -d $(XTBLD) ] ; then \
	    echo Getting bits from HPUX.300.nodebug...; \
	    mkdir $(XTBLD) ;\
	    (cd ../HPUX.300.nodebug ; find . ! -name \*.o \
		| cpio -pduma $(XTBLD)); \
	    echo Replacing dfile.REG with dfile.xterm ;\
	    cp dfile.xterm $(XTBLD)/dfile.REG ;\
	    rm -f $(XTBLD)/hp-ux $(XTBLD)/conf.c ;\
	    sed -e 's;-UQUIETBOOT;-DQUIETBOOT;g' \
		-e 's;-UXTERMINAL;-DXTERMINAL;g' $(XTBLD)/makefile > newmake ;\
	    mv newmake $(XTBLD)/makefile ;\
	 fi



#
#	Build the xtkernel
#
xtkern.300:;
	@echo Building xtkern.300
	-@if [ ! -f .xtkern_only ] ; then \
	    echo Finding xtkern.300 specific .o files ;\
	    find ../[b-z]* $(XTBLD) -name \*.[cs] | \
		xargs egrep -l "XTERMINAL|QUIETBOOT" > .tmp;\
	    LIST= ;\
	    while read f ; do \
		x="$${f##*/}" ;\
		y="$${x%.[cs]}".o ;\
		LIST="$$LIST $$y " ;\
	    done < .tmp;\
	    echo $$LIST > .tmp ;\
	    sed -e 's;locore.o;;g' .tmp > .xtkern_only ;\
	    LIST=`cat .xtkern_only` ;\
	    rm -f .tmp ;\
	    echo Done. ;\
	    echo Building $$LIST ;\
	    ( \
	        cd $(XTBLD) ;\
	        make locore.o $$LIST ;\
	        /usr/local/bin/fixlib $$LIST ;\
	        rm -f hp-ux \
	    ) ;\
	    echo Done. ;\
	else \
	    LIST=`cat .xtkern_only` ;\
	    echo Building $$LIST ;\
	    (cd $(XTBLD) ;\
		make -q locore.o $$LIST ;\
		if [ $$? -ne 0 ] ;then \
		    make locore.o $$LIST ;\
		    /usr/local/bin/fixlib $$LIST ;\
		    rm -f hp-ux ;\
		fi;\
	    );\
	fi
	@(cd $(XTBLD) ; make conf.c ; make -f config.mk)
