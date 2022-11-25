# $Source: /misc/source_product/9.10/commands.rcs/etc/getty/makefile,v $
# $Revision: 66.2.1.5 $

# Common makefile for getty

FLAGS = -USYS_III -DETC_ISSUE -DHANGUP -DMRTS_ON

default: all

all: getty

getty: getty.o
	libs="";						\
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then	\
	    libs="$$libs -lscmd -lsec -lcurses";		\
	fi;							\
	$(CC) -Wl,-a,archive $(CFLAGS) $(LDFLAGS) -o getty getty.o $$libs

clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f getty

#
# Re-define .c.o rule so compiles will use FLAGS macro as well as CFLAGS
#
.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<

# copy /etc/getty to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/getty
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	getty
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp getty ../../../Filesets.sl/UX-CORE/etc
	! chmog 0544 bin bin ../../../Filesets.sl/UX-CORE/etc/getty
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/getty to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/getty
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp getty ../../../Filesets.sl/UX-CORE/etc/getty
	! chmog 0544 bin bin ../../../Filesets.sl/UX-CORE/etc/getty
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
