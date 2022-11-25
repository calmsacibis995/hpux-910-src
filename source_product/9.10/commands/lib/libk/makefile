# @(#) $Revision: 66.6.1.1 $
#
# Common makefile for libk
#

LIB=libk.a

MILLIOBJS= \
    MdivI.o MdivU.o \
    MmulI.o MmulU.o MmuloI.o \
    MremI.o MremU.o MremoI.o \
    MdyncallU.o Mdiv_const.o Mlibc_vers.o

LIBC300 = \
    S300_ldiv.o \
    aldiv.o alrem.o atoi.o auldiv.o aulrem.o  \
    calloc.o cerror.o ctype.o  \
    flag_68881.o fpasmd.o fpshorts.o fpsupport.o fpunsign.o \
    lrem.o rellnt.o rpc_prot.o \
    strcat.o strncat.o strtol.o \
    time.o \
    uldiv.o ulrem.o

VERSION= hpux_rel.o

default: $(LIB)

$(LIB): ../libc/libc.a
	@if hp9000s800; then					\
	    libc_objs="";					\
	    aux_objs="$(MILLIOBJS)";				\
	    echo "\tExtracting objects /lib/milli.a";		\
	    ar x /lib/milli.a $$aux_objs;			\
	else							\
	    libc_objs="$(LIBC300)";				\
	    aux_objs="";					\
	fi;							\
	echo "\tExtracting objects from ../libc/libc.a";	\
	ar x ../libc/libc.a $(VERSION) $$libc_objs;		\
	echo "\tCreating $(LIB)";				\
	/bin/rm -f $(LIB);					\
	ar qc $(LIB) $(VERSION) $$aux_objs $$libc_objs

clean:
	@echo "\tRemoving objects..."
	@rm -f $(MILLIOBJS) $(LIBC300) $(VERSION)

clobber: clean
	@echo "\tRemoving $(LIB)..."
	@rm -f $(LIB)
# copy /lib/libk to the . relative fileset KERN-BLD
# change mode, owner, and group on /lib/libk
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../Filesets.sl/KERN-BLD/etc/conf
	! cp libk.a ../../../Filesets.sl/KERN-BLD/etc/conf/libk.a
	! chmog 0444 bin bin ../../../Filesets.sl/KERN-BLD/etc/conf/libk.a
	! chmog 555 bin bin ../../../Filesets.sl/KERN-BLD/etc/conf
	! chmog 555 bin bin ../../../Filesets.sl/KERN-BLD/etc/
	! chmog 555 bin bin ../../../Filesets.sl/KERN-BLD/
	! chmog 555 bin bin ../../../Filesets.sl/
