# @(#) $Revision: 70.1.1.2 $
#
# makefile for /usr/lib/libio.a
#

CFLAGS= -D_HPUX_SOURCE -D_WSIO

COM_OBJS= io_init.o io_isc.o io_status.o hdw_to_str.o \
	str_to_hdw.o hpux_rel.o
OBJ_700= io_tbl.o

default:
	@if hp9000s300; then			\
	     objs="$(COM_OBJS)";		\
	elif hp9000s700; then			\
	     objs="$(COM_OBJS) $(OBJ_700)";	\
	fi;					\
	$(MAKE) OBJS="$$objs" libio.a


libio.a: $(OBJS)
	ar rv libio.a $(OBJS)

libtest : libio.a libtest.o
	cc -o libtest libtest.o libio.a

libtest.o: libtest.c
	cc -c $(CFLAGS) libtest.c

lintlib:
	lint -o IO -I$(INCL_DIR) -Dvolatile= -D_HPUX_SOURCE llib-lIO.c

clean:
	-/bin/rm -f *.o

clobber: clean
	-/bin/rm -f libio.a
# copy /usr/lib/libio to the . relative fileset BUILDENV
# change mode, owner, and group on /usr/lib/libio
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../../Filesets.sl/BUILDENV/usr/lib
	! cp libio.a ../../../../Filesets.sl/BUILDENV/usr/lib/libio.a
	! chmog 0444 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib/libio.a
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/lib/libio to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/lib/libio
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/lib
	! cp libio.a ../../../../Filesets.sl/PROG-AUX/usr/lib/libio.a
	! chmog 0444 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib/libio.a
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
