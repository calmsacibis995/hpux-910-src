# $Source: /misc/source_product/9.10/commands.rcs/lib/libc/makefile,v $
# @(#) $Revision: 70.12.2.19 $

# Common makefile for libc -- master (top-level) makefile

.SILENT:

COMDIRSX = bsd_x bsdipc_x crt_x csu_x gen_x \
	   net_x netipc_x rpc_x stdio_x stub_x sys_x wpi_x yp_x rel80_x

DIRS300X =
DIRS800X = 
COMDIRSX_SL = $(COMDIRSX:_x=_sl_x)
DIRS300X_SL = $(DIRS300X:_x=_sl_x)
DIRS800X_SL = $(DIRS800X:_x=_sl_x)
COMDIRSX_P = $(COMDIRSX:_x=_p_x)
DIRS300X_P = $(DIRS300X:_x=_p_x)
DIRS800X_P = $(DIRS800X:_x=_p_x)
COMDIRSX_PIC = $(COMDIRSX:_x=_pic_x)
DIRS300X_PIC = $(DIRS300X:_x=_pic_x)
DIRS800X_PIC = $(DIRS800X:_x=_pic_x)

# The next three macros allow individual makes of each subdirectory, of
# either normal, shared, or profiled object files

DIRSX = $(COMDIRSX) $(DIRS300X) $(DIRS800X)
DIRSX_SL = $(COMDIRSX_SL) $(DIRS300X_SL) $(DIRS800X_SL)
DIRSX_P = $(COMDIRSX_P) $(DIRS300X_P) $(DIRS800X_P)
DIRSX_PIC = $(COMDIRSX_PIC) $(DIRS300X_PIC) $(DIRS800X_PIC)


SED = sed
M4 = m4

PREFIX=/lib/pcc_prefix.s

CPPFLAGS = -P
CPPFLG_SL = -P -DPIC
CPPFLG_P = -P -DPROF
CPPFLG_PIC = -P -DPIC

PRFLAGS = -p

#
# Determine appropriate subdirectories to build default target will
# build the normal (non-profiled) version of libc.
#
# Do "make profile" to build only the profiled version, or "make all"
# to build both.
default:	archive shared profile malloc.old

malloc.old:	old/malloc.c
	cd old;						\
	$(MAKE)

archive:
	echo "\tMaking archive libc..."
	if hp9000s800; then				\
	    dirs="$(COMDIRSX) $(DIRS800X)";		\
	elif hp9000s300; then				\
	    dirs="$(COMDIRSX) $(DIRS300X)";		\
	fi;						\
	$(MAKE) DIRSX="$$dirs" libc

shared:
	if ifndef SHARED_LIBS; then			\
	    echo "\tSHARED_LIBS not defined, quiting";	\
	    exit 0;					\
	fi;						\
	echo "\tMaking shared libc...";			\
	if hp9000s800; then				\
	    dirs="$(COMDIRSX_SL) $(DIRS800X_SL)";	\
	elif hp9000s300; then				\
	    dirs="$(COMDIRSX_SL) $(DIRS300X_SL)";	\
	fi;						\
	$(MAKE) DIRSX_SL="$$dirs" libc_sl

profile:
	echo "\tMaking profiled libc..."
	if hp9000s800; then				\
	    dirs="$(COMDIRSX_P) $(DIRS800X_P)";		\
	elif hp9000s300; then				\
	    dirs="$(COMDIRSX_P) $(DIRS300X_P)";		\
	fi;						\
	$(MAKE) DIRSX_P="$$dirs" libc_p

archive_pic:
	echo "\tMaking PIC libc..."
	if hp9000s800; then				\
		dirs="$(COMDIRSX_PIC) $(DIRS800X_PIC)";	\
	elif hp9000s300; then				\
		dirs="$(COMDIRSX_PIC) $(DIRS300X_PIC)";	\
	fi;						\
	$(MAKE) DIRSX_PIC="$$dirs" libc_pic

archive_wrap:
	echo "\tWrapping up libc.a"
	if hp9000s800; then				\
	    dirs="$(COMDIRSX) $(DIRS800X)";		\
	elif hp9000s300; then				\
	    dirs="$(COMDIRSX) $(DIRS300X)";		\
	fi;						\
	$(MAKE) TYPE="" DIRS="$$dirs" wrapup

shared_wrap:
	if ifndef SHARED_LIBS; then			\
	    echo "\tSHARED_LIBS not defined, quiting";	\
	    exit 0;					\
	fi;						\
	echo "\tWrapping up libc.sl and libpicc.a";	\
	if hp9000s800; then				\
	    dirs="$(COMDIRSX_SL) $(DIRS800X_SL)";	\
	elif hp9000s300; then				\
	    dirs="$(COMDIRSX_SL) $(DIRS300X_SL)";	\
	fi;						\
	$(MAKE) TYPE="_sl" DIRS="$$dirs" wrapup
#####	$(MAKE) clobber
#####	$(MAKE) shared

profile_wrap:
	echo "\tWrapping up libc_p.a"
	if hp9000s800; then				\
	    dirs="$(COMDIRSX_P) $(DIRS800X_P)";		\
	elif hp9000s300; then				\
	    dirs="$(COMDIRSX_P) $(DIRS300X_P)";		\
	fi;						\
	$(MAKE) TYPE="_p" DIRS="$$dirs" wrapup

archive_pic_wrap:
	echo "\tWrapping up libpicc.a"
	if hp9000s800; then				\
	    dirs="$(COMDIRSX_PIC) $(DIRS800X_PIC)";	\
	elif hp9000s300; then				\
	    dirs="$(COMDIRSX_PIC) $(DIRS300X_PIC)";	\
	fi;						\
	$(MAKE) TYPE="_pic" DIRS="$$dirs" wrapup

all: archive shared profile archive_pic

libc: $(DIRSX) hpux_rel.o
	answer="`find libc.a -newer tmp -print 2>/dev/null`";	\
	if [ -z "$$answer" ]; then				\
	    /bin/rm -f libc.a;					\
	    $(MAKE) TYPE="" DIRS="$(DIRSX)" wrapup;		\
	else							\
	    echo "\`$@' is up to date.";                        \
	fi

libc_sl: $(DIRSX_SL) hpux_rel.o
	answer="`find libc.sl -newer tmp_sl -print 2>/dev/null`"; \
	if [ -z "$$answer" ]; then				\
	    /bin/rm -f libc.sl libpicc.a;			\
	    $(MAKE) TYPE="_sl" DIRS="$(DIRSX_SL)" wrapup;	\
	else							\
	    echo "\`$@' is up to date.";                        \
	fi

libc_p: $(DIRSX_P) hpux_rel.o
	answer="`find libc_p.a -newer tmp_p -print 2>/dev/null`"; \
	if [ -z "$$answer" ]; then				\
	    /bin/rm -f libc_p.a;				\
	    $(MAKE) TYPE="_p" DIRS="$(DIRSX_P)" wrapup;		\
	else							\
	    echo "\`$@' is up to date.";                        \
	fi

libc_pic: $(DIRSX_PIC) hpux_rel.o
	answer="`find libpicc.a -newer tmp_pic -print 2>/dev/null`"; \
	if [ -z "$$answer" ]; then				\
	    /bin/rm -f libcpicc.a;				\
	    $(MAKE) TYPE="_pic" DIRS="$(DIRSX_PIC)" wrapup;	\
	else							\
	    echo "\`$@' is up to date.";                        \
	fi

#
# wrapup -- Create the "objlist" for a sub-directory.
#           Then use the objlist to create the libraries out of the
#           objects in the tmp$(TYPE) directory.
#
#           Checks to see if all subdirectories completed.  If not, no
#           libraries are made.
#

#
#	The ifdef for lorder is temporary.  It is needed to remove
#	NLS getmsg.o from releases starting in 9.0/IF3.
#	The following line is the permanent lorder line:
#	lorder *.o | tsort > objlist;				\
#

wrapup: hpux_rel.o
	echo "\tChecking sub-directories..."
	dx=`ksh -c 'for i in $(DIRS);do echo $${i%%_*};done'`;	\
	bad=false;						\
	cd tmp$(TYPE);						\
	if [ -f OK.gen1 -a -f OK.gen2 -a			\
	     -f OK.gen3 -a -f OK.gen4 -a			\
	     -f OK.gen5 -a -f OK.gen6 -a			\
	     -f OK.gen7 -a -f OK.gen8 ]; then			\
	    touch OK.gen;					\
	fi;							\
	for i in $$dx; do					\
	    if [ ! -f OK.$$i ]; then				\
		echo "Subdirectory $$i didn't work!" >&2;	\
		bad=true;					\
	    fi;							\
	done;							\
	$$bad && exit 1;					\
	echo "\tCreating object list...";			\
	if hp9000s300; then					\
	    mv fakcu$(TYPE).o fakcu$(TYPE).x;			\
	fi;							\
	mv _exit.o _exit.x;					\
	lorder *.o | tsort | sed "/^getmsg/d" > objlist;	\
	if hp9000s300; then					\
	    mv fakcu$(TYPE).x fakcu$(TYPE).o;			\
	fi;							\
	mv _exit.x _exit.o;					\
	out=../libc$(TYPE).a;					\
	if [ "$(TYPE)" = "_sl" ]; then				\
	    out=../libpicc.a;					\
	fi;							\
	if hp9000s300; then					\
	    if [ "$(TYPE)" = "_sl" ]; then			\
		tailobjs="_exit.o";				\
	    else						\
		tailobjs="fakcu$(TYPE).o _exit.o";		\
	    fi;							\
	elif hp9000s800; then					\
	    tailobjs="_exit.o";					\
	fi;							\
	echo "\tCreating archive library...";			\
	rm -f $$out;						\
	ar qc $$out ../hpux_rel.o `cat objlist` $$tailobjs;	\
	if [ "$(TYPE)" = "_sl" ]; then				\
	    echo "\tCreating shared library...";		\
	    rm -f ../libc.sl;					\
	    ld -x -b -o ../libc.sl ../hpux_rel.o `cat objlist`	\
				     $$tailobjs;		\
	    sleep 1; touch ../libc.sl $$out;			\
	fi

#
# For each subdirectory_x, truncate the _x suffix, then make the subdir
#
$(DIRSX) gen1_x gen2_x gen3_x gen4_x \
	 gen5_x gen6_x gen7_x gen8_x:
	mkdir tmp >/dev/null 2>&1 || exit 0
	dir=$(@:_x=);						\
	if [ -f "tmp/OK.$$dir" ]; then				\
	    echo "\`$@' is up to date.";			\
	    exit 0;						\
	fi;							\
	part="all";						\
	case "$$dir" in						\
	gen[123456789])						\
	    part=`expr substr $$dir 4 1`;			\
	    dir=gen;						\
	    ;;							\
	esac;							\
	if [ "$$part" != "all" ]; then				\
	    echo "\tMaking part $$part of regular objects in $$dir"; \
	else							\
	    echo "\tMaking regular objects in $$dir";		\
	fi;							\
	cd $$dir;						\
	m4flags="";						\
	ifdef _NAMESPACE_CLEAN &&				\
	    m4flags="$$m4flags -D_NAMESPACE_CLEAN";		\
	ifdef NLS &&						\
	    m4flags="$$m4flags -DNLS";				\
	$(MAKE) M4="$(M4)"					\
		M4FLAGS="$$m4flags"				\
		SED="$(SED)"					\
		PREFIX="$(PREFIX)"				\
		ASFLAGS=""					\
		CPPFLAGS="$(CPPFLAGS)"				\
		PART="$$part"					\
		archive

#
# For each subdirectory_sl_x, truncate the _sl_x suffix, then make the
# shared version of the subdir
#
$(DIRSX_SL) gen1_sl_x gen2_sl_x gen3_sl_x gen4_sl_x \
	    gen5_sl_x gen6_sl_x gen7_sl_x gen8_sl_x:
	if ifndef SHARED_LIBS; then				\
	    echo "\tSHARED_LIBS not defined, quiting";		\
	    exit 0;						\
	fi;							\
	( mkdir tmp_sl >/dev/null 2>&1 || exit 0 );		\
	dir=$(@:_sl_x=);					\
	if [ -f "tmp_sl/OK.$$dir" ]; then			\
	    echo "\`$@' is up to date.";			\
	    exit 0;						\
	fi;							\
	part="all";						\
	case "$$dir" in						\
	gen[123456789])						\
	    part=`expr substr $$dir 4 1`;			\
	    dir=gen;						\
	    ;;							\
	esac;							\
	if [ "$$part" != "all" ]; then				\
	    echo "\tMaking part $$part of shared objects in $$dir"; \
	else							\
	    echo "\tMaking shared objects in $$dir";		\
	fi;							\
	cd $$dir;						\
	m4flags="-DPIC";					\
	ifdef _NAMESPACE_CLEAN &&				\
	    m4flags="$$m4flags -D_NAMESPACE_CLEAN";		\
	ifdef NLS &&						\
	    m4flags="$$m4flags -DNLS";				\
	$(MAKE) M4="$(M4)"					\
		M4FLAGS="$$m4flags"				\
		SED="$(SED)"					\
		PREFIX="$(PREFIX)"				\
		ASFLAGS="+z"					\
		CFLAGS="-DPIC +z"				\
		CPPFLAGS="$(CPPFLG_SL)"				\
		PART="$$part"					\
		shared

#
# For each subdirectory_p_x, truncate the _p_x suffix, then make the
# profiled version of the subdir
#
$(DIRSX_P) gen1_p_x gen2_p_x gen3_p_x gen4_p_x \
	   gen5_p_x gen6_p_x gen7_p_x gen8_p_x:
	mkdir tmp_p >/dev/null 2>&1 || exit 0
	dir=$(@:_p_x=);						\
	if [ -f "tmp_p/OK.$$dir" ]; then			\
	    echo "\`$@' is up to date.";			\
	    exit 0;						\
	fi;							\
	part="all";						\
	case "$$dir" in						\
	gen[123456789])						\
	    part=`expr substr $$dir 4 1`;			\
	    dir=gen;						\
	    ;;							\
	esac;							\
	if [ "$$part" != "all" ]; then				\
	    echo "\tMaking part $$part of profiled objects in $$dir"; \
	else							\
	    echo "\tMaking profiled objects in $$dir";		\
	fi;							\
	cd $$dir;						\
	m4flags="-DPROFILE";					\
	ifdef _NAMESPACE_CLEAN &&				\
	    m4flags="$$m4flags -D_NAMESPACE_CLEAN";		\
	ifdef NLS &&						\
	    m4flags="$$m4flags -DNLS";				\
	$(MAKE) M4="$(M4)"					\
		M4FLAGS="$$m4flags"				\
		SED="$(SED)"					\
		PREFIX="$(PREFIX)"				\
		ASFLG_P=""					\
		CPPFLG_P="$(CPPFLG_P)"				\
		PRFLAGS="$(PRFLAGS)"				\
		PART="$$part"					\
		profile

#
# For each subdirectory_pic_x, truncate the _pic_x suffix, then make the
# archive PIC version of the subdir
#
$(DIRSX_PIC) gen1_pic_x gen2_pic_x gen3_pic_x gen4_pic_x \
	     gen5_pic_x gen6_pic_x gen7_pic_x gen8_pic_x:
	mkdir tmp_pic >/dev/null 2>&1 || exit 0
	dir=$(@:_pic_x=);					\
	if [ -f "tmp_pic/OK.$$dir" ]; then			\
	    echo "\`$@' is up to date.";			\
	    exit 0;						\
	fi;							\
	part="all";						\
	case "$$dir" in						\
	gen[123456789])						\
	    part=`expr substr $$dir 4 1`;			\
	    dir=gen;						\
	    ;;							\
	esac;							\
	if [ "$$part" != "all" ]; then				\
	    echo "\tMaking part $$part of PIC objects in $$dir"; \
	else							\
	    echo "\tMaking PIC objects in $$dir";		\
	fi;							\
	cd $$dir;						\
	m4flags="-DPIC";					\
	ifdef _NAMESPACE_CLEAN &&				\
	    m4flags="$$m4flags -D_NAMESPACE_CLEAN";		\
	ifdef NLS &&						\
	    m4flags="$$m4flags -DNLS";				\
	$(MAKE) M4="$(M4)"					\
		M4FLAGS="$$m4flags"				\
		SED="$(SED)"					\
		PREFIX="$(PREFIX)"				\
		ASFLAGS="+Z"					\
		CFLAGS="-DPIC +Z"				\
		CPPFLAGS="$(CPPFLG_PIC)"			\
		PART="$$part"					\
		archive_pic

clean:
	echo "\tCleaning..."
	if hp9000s800; then				\
	    dirs="$(COMDIRSX:_x=) $(DIRS800X:_x=)";	\
	elif hp9000s300; then				\
	    dirs="$(COMDIRSX:_x=) $(DIRS300X:_x=)";	\
	fi;						\
	startdir=`pwd`;					\
	for i in $$dirs;				\
	do 						\
	    cd $$i; $(MAKE) clean; 			\
	    cd $$startdir; 				\
	done
	/bin/rm -rf tmp tmp_sl tmp_p tmp_pic hpux_rel.o

clobber: clean
	echo "\tClobbering..."
	rm -f libc.a libc_p.a libc.sl libpicc.a csu/*.o gen/perror.cat \
	      gen/getopt_lib.cat gen/regcomp.cat
# copy /lib/libc to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/libc
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	! cp csu/crt0.o ../../../Filesets.sl/BUILDENV/lib/crt0.o
	! chmog 0444 bin bin ../../../Filesets.sl/BUILDENV/lib/crt0.o
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libc to the . relative fileset C-MIN
# change mode, owner, and group on /lib/libc
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C-MIN/lib
	! cp csu/crt0.o ../../../Filesets.sl/C-MIN/lib/crt0.o
	! chmog 0444 bin bin ../../../Filesets.sl/C-MIN/lib/crt0.o
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/lib
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libc to the . relative fileset PROG-MIN
# change mode, owner, and group on /lib/libc
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/PROG-MIN/lib
	! cp csu/gcrt0.o ../../../Filesets.sl/PROG-MIN/lib/gcrt0.o
	! chmog 0444 bin bin ../../../Filesets.sl/PROG-MIN/lib/gcrt0.o
	! chmog 555 bin bin ../../../Filesets.sl/PROG-MIN/lib
	! chmog 555 bin bin ../../../Filesets.sl/PROG-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libc to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/libc
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	! cp libc.a ../../../Filesets.sl/BUILDENV/lib/libc.a
	! chmog 0444 bin bin ../../../Filesets.sl/BUILDENV/lib/libc.a
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libc to the . relative fileset C-MIN
# change mode, owner, and group on /lib/libc
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C-MIN/lib
	! cp libc.a ../../../Filesets.sl/C-MIN/lib/libc.a
	! chmog 0444 bin bin ../../../Filesets.sl/C-MIN/lib/libc.a
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/lib
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libc to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/libc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	! cp libc.sl ../../../Filesets.sl/BUILDENV/lib/libc.sl
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/lib/libc.sl
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# create the libc.sl for XTERM300
	mkdir -p ../../../Filesets.sl/XTERM300/usr/xterm300/lib
	! cp libc.sl ../../../Filesets.sl/XTERM300/usr/xterm300/lib/libc.sl
	! chmog 0555 bin bin ../../../Filesets.sl/XTERM300/usr/xterm300/lib/libc.sl
	! chmog 0555 bin bin ../../../Filesets.sl/XTERM300/usr/xterm300/lib
	! chmog 0555 bin bin ../../../Filesets.sl/XTERM300/usr/xterm300
	! chmog 0555 bin bin ../../../Filesets.sl/XTERM300/usr
	! chmog 0555 bin bin ../../../Filesets.sl/XTERM300
# copy /lib/libc to the . relative fileset CORE-SHLIBS
# change mode, owner, and group on /lib/libc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CORE-SHLIBS/lib
	! cp libc.sl ../../../Filesets.sl/CORE-SHLIBS/lib/libc.sl
	! chmog 0555 bin bin ../../../Filesets.sl/CORE-SHLIBS/lib/libc.sl
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/lib
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libc to the . relative fileset PROG-AUX
# change mode, owner, and group on /lib/libc
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/PROG-AUX/lib/libp
	! cp libc_p.a ../../../Filesets.sl/PROG-AUX/lib/libp/libc.a
	! chmog 0444 bin bin ../../../Filesets.sl/PROG-AUX/lib/libp/libc.a
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/lib/libp
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/lib/
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libc to the . relative fileset PROG-MIN
# change mode, owner, and group on /lib/libc
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/PROG-MIN/lib
	! cp csu/mcrt0.o ../../../Filesets.sl/PROG-MIN/lib/mcrt0.o
	! chmog 0444 bin bin ../../../Filesets.sl/PROG-MIN/lib/mcrt0.o
	! chmog 555 bin bin ../../../Filesets.sl/PROG-MIN/lib
	! chmog 555 bin bin ../../../Filesets.sl/PROG-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libc to the . relative fileset NLS-CORE
# change mode, owner, and group on /lib/libc
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! cp gen/getopt_lib.cat ../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/getopt_lib.cat
	! chmog 0444 bin bin ../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/getopt_lib.cat
	! chmog 555 bin bin ../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/NLS-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/NLS-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libc to the . relative fileset CORE-SHLIBS
# change mode, owner, and group on /lib/libc
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CORE-SHLIBS/usr/lib/nls/C
	! cp gen/perror.cat ../../../Filesets.sl/CORE-SHLIBS/usr/lib/nls/C/perror.cat
	! chmog 0444 bin bin ../../../Filesets.sl/CORE-SHLIBS/usr/lib/nls/C/perror.cat
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libc to the . relative fileset NLS-CORE
# change mode, owner, and group on /lib/libc
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! cp gen/regcomp.cat ../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/regcomp.cat
	! chmog 0444 bin bin ../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/regcomp.cat
	! chmog 555 bin bin ../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/NLS-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/NLS-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libc to the . relative fileset CORE-SHLIBS
# change mode, owner, and group on /lib/libc
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CORE-SHLIBS/usr/lib/nls/C
	! ln ../../../Filesets.sl/CORE-SHLIBS/usr/lib/nls/C/perror.cat ../../../Filesets.sl/CORE-SHLIBS/usr/lib/nls/C/strerror.cat
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/usr/
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libc to the . relative fileset C-MIN
# change mode, owner, and group on /lib/libc
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C-MIN/usr/old
	! cp tmp/getmsg.o ../../../Filesets.sl/C-MIN/usr/old/getmsg.o
	! cp old/malloc.o ../../../Filesets.sl/C-MIN/usr/old/malloc3c.o
	! chmog 0444 bin bin ../../../Filesets.sl/C-MIN/usr/old/getmsg.o
	! chmog 0444 bin bin ../../../Filesets.sl/C-MIN/usr/old/malloc3c.o
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/usr/old
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
