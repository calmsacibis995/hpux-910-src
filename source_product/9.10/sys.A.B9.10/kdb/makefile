# To produce:					Type:
#
#	SYSDEBUG                                	make
#	CDBKDB						make CDBKDB
#
# When switching from one to another of the above (as opposed to an
# incremental build of the same target), do a "make clean" as the
# first step.

SHELL		= /bin/sh

CC		= cc
COMMON_FLAGS	= -DCMD_EDIT -UKDBKDB -UDEBUG -DSDS
CFLAGS		= +O1 +M -W2,-i -I../Filesets/BUILDENV/usr/include
CDBKDB_CFLAGS	= -DCDBKDB -UCMD_EDIT -g 

LD		= ld
LFLAGS		= -a archive

# How to do a generic compile to a .o file:
.c.o:
	$(CC) $(COMMON_FLAGS) $(SELECT_CODE) $(CFLAGS) -c $*.c

KDB_files_1	= Kasm.o Kprintf.o Kscanf.o
KDB_files_2	= Ksys.o Ktty.o hil.o graphics.o Ksym.o Kins.o
CDB_files	= bp.o cmd.o expr.o files.o format.o hpux_rel.o list.o \
		  main.o pt.o single.o slt.o sym.o tk.o ty.o var.o

ofiles		= $(KDB_files_1) $(KDB_files_2) $(CDB_files)

default:	SYSDEBUG



SYSDEBUG:	ext.h mvtd $(ofiles)
	$(LD) $(LFLAGS) -r -d -o SYSDEBUG $(ofiles) -lc
	$(LD) $(LFLAGS) -N -o SYSTEXT $(ofiles) -lc
#	$(LD) $(LFLAGS) -r -d -o SYSDEBUG $(ofiles) ./libc.a
#	$(LD) $(LFLAGS) -N -o SYSTEXT $(ofiles)  ./libc.a
	./mvtd SYSTEXT SYSDEBUG
	@echo "SYSDEBUG built - KDB MAKEFILE COMPLETE"
#	$(LD) $(LFLAGS) +r -N -o SYS+R $(ofiles) -lc

CDBKDB:		_cdbkdb
	@$(MAKE) CFLAGS="$(CDBKDB_CFLAGS)" _CDBKDB

_CDBKDB:	ext.h $(KDB_files_2) $(CDB_files)
	$(CC) -o CDBKDB $(KDB_files_2) $(CDB_files)
	@echo "CDBKDB built - KDB MAKEFILE COMPLETE"

_cdbkdb:
	rm -f CDBKDB


# Note:  Depending upon what is changing in a particular release, you
#	 may not be able to compile mvtd using the Build Environment
#	 compiler/linker because the resulting executible may only
#	 run properly on the target system while you need to run mvtd
#	 on the build system.  If you run into this problem copy 
#	 a.out.h and sys/magic.h from the Build Environment to the
#	 current directory and build mvtd (using the current system's
#	 compiler):
#
#		cc -I. -o mvtd mvtd
#
mvtd:
	cc -Wl,-a,archive -o mvtd mvtd.c

Kasm.o:
	cat Kasm.s | /lib/cpp $(COMMON_FLAGS) > Kasmtmp.s
	as -o Kasm.o Kasmtmp.s 
	rm Kasmtmp.s

$(ofiles): ext.h

# NOTE: If you modify (or add) any procedure formals, force this rule to
# 	execute by removing ext.h
# NOTE: ext.sed leaves out procedures of type "void" so things compile faster.
ext.h:
	rm -f $@
	grep '^export' *.c | sed -f ext.sed >$@
	chmod -w $@

clean:
	rm -f $(ofiles) mvtd ext.h

clobber: clean
	rm -f SYSDEBUG CDBKDB

lint:	ext.h
	lint -Zn4096 $(COMMON_FLAGS) *.c


# Build SYSDEBUG using special version of ld with the "+r" option.
# The ld +r option allows us to build a kdb linked to a libc.a that
# contains PIC.  The special version of ld must exist in the current
# path with the name "ldr".
#
special:	_SPECIAL
	@$(MAKE) COMMON_FLAGS="$(COMMON_FLAGS) -DSPECIAL_LD" _special

_special: ext.h $(ofiles)
	ldr $(LFLAGS) +r -N -o SYSDEBUG $(ofiles) -lc
	@echo "SYSDEBUG built with PIC libc.a - KDB MAKEFILE COMPLETE"

_SPECIAL:
	rm -f SYSDEBUG
