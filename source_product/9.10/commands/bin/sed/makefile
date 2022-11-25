# $Source: /misc/source_product/9.10/commands.rcs/bin/sed/makefile,v $
# $Revision: 66.1.1.4 $

# Common makefile for sed

SOURCES  = sed0.c sed1.c hpux_rel.c
OBJECTS  = sed0.o sed1.o hpux_rel.o
DEBUG	 = debug.sed
PFLAGS   = -DEBUG=1

default: 	sed

sed:  	$(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) 

$(OBJECTS):  sed.h

$(DEBUG):  clean
	$(CC) $(CFLAGS) $(LDFLAGS) $(PFLAGS) -o $(DEBUG) $(SOURCES)
	/bin/rm -f $(OBJECTS) 

clean:
	/bin/rm -f $(OBJECTS) 

clobber: clean
	/bin/rm -f sed $(DEBUG)

# copy /bin/sed to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sed
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	sed
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp sed ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/sed
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/sed to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sed
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp sed ../../../Filesets.sl/UX-CORE/bin/sed
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/sed
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
