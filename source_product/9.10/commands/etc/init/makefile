# @(#) $Revision: 66.3.1.6 $
#
# makefile -- makefile for /etc/init
#
# To make a "user process" version of init, set
#  DFLAGS=-DUDEBUG -DDEBUG1 -DDEBUG -DACCTDEBUG -DERRDEBUG
#
# MEMORY_CHECKS cause extra code to be included that checks if
#               init is allocating and freeing resources correctly.
#               If an error is detected, the file "init.memory" will
#               be created with diagnostics.
#               Defining DEBUG1 will increase the frequency of the
#               memory checks.
#
# DFLAGS  =
# DFLAGS  = -DMEMORY_CHECKS
# DFLAGS  = -g -DUDEBUG -DDEBUG1 -DDEBUG -DACCTDEBUG -DERRDEBUG -DMEMORY_CHECKS

DFLAGS  =
OBJECTS = \
	account.o	\
	init.o		\
	inittab.o	\
	proctable.o	\
	set_siteid.o	\
	unsp.o		\
	unsp_open.o	\
	uroutines.o	\
	utility.o	\
	hpux_rel.o
FLAGS   = $(DFLAGS)

default: init

init: $(OBJECTS)
	libs="";						\
	if ifdef DISKLESS || ifdef DUX; then			\
	    libs="$$libs -lDUX";				\
	fi;							\
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then	\
	    libs="$$libs -lscmd -lsec -lcurses";		\
	fi;							\
	$(CC) -Wl,-a,archive -o $@ $(CFLAGS) $(OBJECTS) $$libs $(LDFLAGS)

clean:
	/bin/rm -f *.o _tmp.s

clobber: clean
	/bin/rm -f init

.c.o:
	$(CC) -c $(CFLAGS) $(FLAGS) $*.c

#
# Assemble a ".s" file with cpp directives in a machine independent
# manner
#
.s.o:
	$(CC) -E $(CFLAGS) $(FLAGS) $*.s >_tmp.s
	$(CC) -c $(CFLAGS) $(FLAGS) _tmp.s
	mv _tmp.o $*.o
	/bin/rm -f _tmp.s

hpux_rel.o: hpux_rel.c
	set -- $(DFLAGS);					     \
	extra="";						     \
	for i in $$*; do					     \
	    case $$i in						     \
	    -DMEMORY_CHECKS|-DUDEBUG|-g)			     \
		if [ -z "$$extra" ]; then			     \
		    extra="$$i";				     \
		else						     \
		    extra="$$extra $$i";			     \
		fi;						     \
		;;						     \
	    *)							     \
		;;						     \
	    esac;						     \
	done;							     \
	if [ -n "$$extra" ]; then				     \
	    sed -e 's/ \$$"/ ('"$$extra"') $$"/' <hpux_rel.c >tmp.c; \
	    $(CC) -c $(CFLAGS) $(FLAGS) tmp.c -o hpux_rel.o;	     \
	    rm -f tmp.c;					     \
	else							     \
	    $(CC) -c $(CFLAGS) $(FLAGS) hpux_rel.c;		     \
	fi

init.c:      init.h proctable.h
proctable.c: init.h proctable.h
utility.c:   init.h
inittab.c:   init.h

# copy /etc/init to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/init
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	init
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp init ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/init
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/init to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/init
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp init ../../../Filesets.sl/UX-CORE/etc/init
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/init
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/init to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/init
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! ln ../../../Filesets.sl/UX-CORE/etc/init ../../../Filesets.sl/UX-CORE/etc/telinit
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
