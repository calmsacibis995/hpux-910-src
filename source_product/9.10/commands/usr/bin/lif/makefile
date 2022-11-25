# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/lif/makefile,v $
# $Revision: 66.1.1.5 $

# This is the common makefile for the LIF utilities.
# It will make lifcp, lifinit, lifls, lifrename and lifrm.

OBJECTS=	findvsize.o gettime.o lfindfile.o lifclose.o \
		lifcp.o lifcreate.o lifinit.o lifls.o lifopen.o \
		lifpurge.o lifrm.o lifrn.o lifspace.o lifvol.o \
		volsize.o movesects.o unitread.o unitwrite.o

INCLUDE=	lifdef.h global.h

# Objects that are needed for lifcp.
OCP=	lifcp.o lifopen.o \
	lifcreate.o lifclose.o \
	lfindfile.o gettime.o movesects.o \
	volsize.o lifvol.o \
	unitread.o unitwrite.o lifspace.o lifpurge.o \
	findvsize.o bugout.o \
	hpux_rel.o

# Objects that are needed for lifinit.
OINIT=	lifinit.o \
	volsize.o lifvol.o \
	unitread.o unitwrite.o \
	gettime.o findvsize.o bugout.o \
	hpux_rel.o

# Objects that are needed for lifls.
OLS=	lifls.o \
	volsize.o lifvol.o \
	unitread.o unitwrite.o findvsize.o bugout.o \
	hpux_rel.o

# Objects that are needed for lifrm.
ORM=	lifrm.o lifpurge.o\
	findvsize.o volsize.o lifvol.o \
	unitread.o unitwrite.o \
	lfindfile.o  bugout.o \
	hpux_rel.o

# Objects that are needed for lifrename.
ORENAME=	lifrn.o lifpurge.o\
		findvsize.o volsize.o lifvol.o \
		unitread.o unitwrite.o \
		lfindfile.o  bugout.o \
		hpux_rel.o

default: all

all:	lifcp lifinit lifls lifrename lifrm

lifcp:	$(OCP)
	$(CC) $(LDFLAGS) -Wl,-a,archive -o lifcp $(OCP)

lifinit:$(OINIT)
	$(CC) $(LDFLAGS) -o lifinit  $(OINIT)

lifls:  $(OLS)	
	$(CC) $(LDFLAGS) -o lifls $(OLS)

lifrename:	$(ORENAME)
	$(CC) $(LDFLAGS) -o lifrename $(ORENAME) 

lifrm:	$(ORM)
	$(CC) $(LDFLAGS) -Wl,-a,archive -o lifrm $(ORM) 

# If make is called with the lint target then the the above files (names will
# be changed from .o's to .c's) will be run through lint.
lint: lifcp.ln lifinit.ln lifls.ln lifrn.ln lifrm.ln

lifcp.ln: $(OCP:.o=.c)
	exec lint $(OCP:.o=.c) >$@

lifinit.ln: $(OINIT:.o=.c)
	exec lint $(OINIT:.o=.c) >$@

lifls.ln: $(OLS:.o=.c)
	exec lint $(OLS:.o=.c) >$@

lifrn.ln: $(ORENAME:.o=.c)
	exec lint $(ORENAME:.o=.c) >$@

lifrm.ln: $(ORM:.o=.c)
	exec lint $(ORM:.o=.c) >$@

clean:
	/bin/rm -f *.o *.ln
	/bin/rm -f mon.out 

clobber: clean
	/bin/rm -f lifcp lifinit lifls lifrm lifrename

$(OBJECTS):	$(INCLUDE)
# copy /usr/bin/lif to the . relative fileset KERN-BLD
# change mode, owner, and group on /usr/bin/lif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/KERN-BLD/usr/bin
	! cp lifcp ../../../../Filesets.sl/KERN-BLD/usr/bin/lifcp
	! chmog 0555 bin bin ../../../../Filesets.sl/KERN-BLD/usr/bin/lifcp
	! chmog 555 bin bin ../../../../Filesets.sl/KERN-BLD/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/KERN-BLD/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/KERN-BLD/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lif to the . relative fileset KERN-BLD
# change mode, owner, and group on /usr/bin/lif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/KERN-BLD/usr/bin
	! cp lifinit ../../../../Filesets.sl/KERN-BLD/usr/bin/lifinit
	! chmog 0555 bin bin ../../../../Filesets.sl/KERN-BLD/usr/bin/lifinit
	! chmog 555 bin bin ../../../../Filesets.sl/KERN-BLD/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/KERN-BLD/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/KERN-BLD/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lif to the . relative fileset KERN-BLD
# change mode, owner, and group on /usr/bin/lif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/KERN-BLD/usr/bin
	! cp lifls ../../../../Filesets.sl/KERN-BLD/usr/bin/lifls
	! chmog 0555 bin bin ../../../../Filesets.sl/KERN-BLD/usr/bin/lifls
	! chmog 555 bin bin ../../../../Filesets.sl/KERN-BLD/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/KERN-BLD/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/KERN-BLD/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lif to the . relative fileset KERN-BLD
# change mode, owner, and group on /usr/bin/lif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/KERN-BLD/usr/bin
	! cp lifrename ../../../../Filesets.sl/KERN-BLD/usr/bin/lifrename
	! chmog 0555 bin bin ../../../../Filesets.sl/KERN-BLD/usr/bin/lifrename
	! chmog 555 bin bin ../../../../Filesets.sl/KERN-BLD/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/KERN-BLD/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/KERN-BLD/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/lif to the . relative fileset KERN-BLD
# change mode, owner, and group on /usr/bin/lif
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/KERN-BLD/usr/bin
	! cp lifrm ../../../../Filesets.sl/KERN-BLD/usr/bin/lifrm
	! chmog 0555 bin bin ../../../../Filesets.sl/KERN-BLD/usr/bin/lifrm
	! chmog 555 bin bin ../../../../Filesets.sl/KERN-BLD/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/KERN-BLD/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/KERN-BLD/
	! chmog 555 bin bin ../../../../Filesets.sl/
