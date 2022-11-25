# @(#) $Revision: 70.13 $
#
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/makefile,v $
#
# makefile for libm: transcendentals and other math functions.
#
# Profiling will be done with results placed in libm_p.a
#

M4	      = m4 -B10000
CC	      = cc
LIBm          = libm.a
LIBm_P	      = libm_p.a
LIBm_SL       = libm.sl
LIBm_PIC      = libmpic.a
LIBM          = libM.a
LIBM_P        = libM_p.a
LIBM_SL       = libM.sl
LIBM_PIC      = libMpic.a

SPFLAGS       = -Wc,-Yf


#
# CFLAGS doesn't have -D_NAMESPACE_CLEAN since it is/isn't in CCOPTS
#
CFLAGS        = 
M4FLAGS       = -D_NAMESPACE_CLEAN

#
# Revision stamp (included in all flavors of the libraries)
#
REL	      = hpux_rel.o

#
# DPOBJECTS contains routines that don't receive arguments
#           of type (float)
# SPOBJECTS contains routines that receive arguments of type (float)
#           and therefore need to be compiled with flags SPFLAGS
# ASOBJECTS contains routines that are written in 68K family
#           assembly language
# AS80OBJECTS contains routines that are written in 68K family
#           assembly language and are put in the shared libraries only
#           for backwards compatibility

DPOBJECTS     = \
	erf.o		\
	error_msg.o	\
	matherr.o	\
	fabs.o		\
	gamma.o		\
	hypot.o		\
	isinf.o		\
	isnan.o		\
	j0.o		\
	j1.o		\
	jn.o

SPOBJECTS     = \
	rfuncs.o

ASOBJECTS     = \
	a_asin.o	\
	a_atan.o	\
	a_atan2.o	\
	a_cosh.o	\
	a_exp.o		\
	a_fasin.o	\
	a_fatan.o	\
	a_fatan2.o	\
	a_fcosh.o	\
	a_fexp.o	\
	a_flog.o	\
	a_floor.o	\
	a_fmod.o	\
	a_fpow.o	\
	a_fsin.o	\
	a_fsinh.o	\
	a_fsqrt.o	\
	a_ftan.o	\
	a_ftanh.o	\
	a_log.o		\
	a_pow.o		\
	a_sin.o		\
	a_sinh.o	\
	a_sqrt.o	\
	a_tan.o		\
	a_tanh.o	\
	fp040_exp.o	\
	fp040_log.o	\
	fp040_trig.o	\
	fp040_atrig.o \
	fp040_mod.o \
	fp040_misc.o

AS80OBJECTS     = \
	a_asin80.o	\
	a_atan80.o	\
	a_atan280.o	\
	a_cosh80.o	\
	a_exp80.o	\
	a_fasin80.o	\
	a_fatan80.o	\
	a_fatan280.o	\
	a_fcosh80.o	\
	a_fexp80.o	\
	a_flog80.o	\
	a_floor80.o	\
	a_fmod80.o	\
	a_fsin80.o	\
	a_fsinh80.o	\
	a_ftan80.o	\
	a_ftanh80.o	\
	a_log80.o	\
	a_sin80.o	\
	a_sinh80.o	\
	a_tan80.o	\
	a_tanh80.o


DPOBJECTS_P    = $(DPOBJECTS:.o=_p.o)
DPOBJECTS_SL   = $(DPOBJECTS:.o=_sl.o)
DPOBJECTS_M    = $(DPOBJECTS:.o=M.o)
DPOBJECTS_M_P  = $(DPOBJECTS:.o=M_p.o)
DPOBJECTS_M_SL = $(DPOBJECTS:.o=M_sl.o)

SPOBJECTS_P    = $(SPOBJECTS:.o=_p.o)
SPOBJECTS_SL   = $(SPOBJECTS:.o=_sl.o)
SPOBJECTS_M    = $(SPOBJECTS:.o=M.o)
SPOBJECTS_M_P  = $(SPOBJECTS:.o=M_p.o)
SPOBJECTS_M_SL = $(SPOBJECTS:.o=M_sl.o)

ASOBJECTS_P    = $(ASOBJECTS:.o=_p.o)
ASOBJECTS_SL   = $(ASOBJECTS:.o=_sl.o)
ASOBJECTS_M    = $(ASOBJECTS:.o=M.o)
ASOBJECTS_M_P  = $(ASOBJECTS:.o=M_p.o)
ASOBJECTS_M_SL = $(ASOBJECTS:.o=M_sl.o)

AS80OBJECTS_SL   = $(AS80OBJECTS:.o=_sl.o)
AS80OBJECTS_M_SL = $(AS80OBJECTS:.o=M_sl.o)

KROBJECTS_P    = $(KROBJECTS:.o=_p.o)
KROBJECTS_SL   = $(KROBJECTS:.o=_sl.o)

default: archive shared profile

archive: archivem archiveM

shared: sharedm sharedM

profile: profilem profileM
	
archivem: $(LIBm)

archiveM: $(LIBM)

sharedm sharedM:
	@if ifndef SHARED_LIBS; then			\
	    echo "\tSHARED_LIBS not defined, quiting";	\
	    exit 0;					\
	fi;						\
	if [ "$@" = sharedm ]; then			\
	    $(MAKE) $(LIBm_SL);				\
	else						\
	    $(MAKE) $(LIBM_SL);				\
	fi

profilem: $(LIBm_P)

profileM: $(LIBM_P)
	
#
# Creates libm.a
#
$(LIBm): $(DPOBJECTS) $(SPOBJECTS) $(ASOBJECTS) $(KROBJECTS) $(REL)
	@/bin/rm -f $@
	@echo "\tCreating $@..."
	@ar qc $@ $(REL) `lorder $(DPOBJECTS) $(SPOBJECTS) \
			         $(ASOBJECTS) $(KROBJECTS) | tsort`

#
# Creates libM.a
#
$(LIBM): $(DPOBJECTS_M) $(SPOBJECTS_M) $(ASOBJECTS_M) $(REL)
	@/bin/rm -f $@
	@echo "\tCreating $@..."
	@ar qc $@ $(REL) `lorder $(DPOBJECTS_M) $(SPOBJECTS_M) \
			         $(ASOBJECTS_M) | tsort`

#
# Creates libm.sl and libmpic.a
#
$(LIBm_SL): $(DPOBJECTS_SL) $(SPOBJECTS_SL) \
	    $(ASOBJECTS_SL) $(AS80OBJECTS_SL) $(KROBJECTS_SL) $(REL)
	@/bin/rm -f $@
	@echo "\tCreating $(LIBm_PIC)..."
	@ar qc $(LIBm_PIC) $(REL) \
	        `lorder $(DPOBJECTS_SL) $(SPOBJECTS_SL) \
			$(ASOBJECTS_SL) $(AS80OBJECTS_SL) \
			$(KROBJECTS_SL) | tsort`
	@echo "\tCreating $@..."
	@ld -b -o $@ $(REL) $(DPOBJECTS_SL) $(SPOBJECTS_SL) \
			    $(ASOBJECTS_SL) $(AS80OBJECTS_SL) \
			    $(KROBJECTS_SL)

#
# Creates libM.sl and libMpic.a
#
$(LIBM_SL): $(DPOBJECTS_M_SL) $(SPOBJECTS_M_SL) \
	    $(ASOBJECTS_M_SL) $(AS80OBJECTS_M_SL) $(REL)
	@/bin/rm -f $(LIBM_PIC) $@
	@echo "\tCreating $(LIBM_PIC)..."
	@ar qc $(LIBM_PIC) $(REL) \
	        `lorder $(DPOBJECTS_M_SL) $(SPOBJECTS_M_SL) \
		        $(ASOBJECTS_M_SL) $(AS80OBJECTS_M_SL) | tsort`
	@echo "\tCreating $@..."
	@ld -b -o $@ $(REL) $(DPOBJECTS_M_SL) $(SPOBJECTS_M_SL) \
			    $(ASOBJECTS_M_SL) $(AS80OBJECTS_M_SL)

#
# Creates libm_p.a
#
$(LIBm_P): $(DPOBJECTS_P) $(SPOBJECTS_P) \
	   $(ASOBJECTS_P) $(KROBJECTS_P) $(REL)
	@/bin/rm -f $@
	@echo "\tCreating $@..."
	@ar qc $@ $(REL) `lorder $(DPOBJECTS_P) $(SPOBJECTS_P) \
			         $(ASOBJECTS_P) $(KROBJECTS_P) | tsort`

#
# Creates libm_p.a
#
$(LIBM_P): $(DPOBJECTS_M_P) $(SPOBJECTS_M_P) $(ASOBJECTS_M_P) $(REL)
	@/bin/rm -f $@
	@echo "\tCreating $@..."
	@ar qc $@ $(REL) `lorder $(DPOBJECTS_M_P) $(SPOBJECTS_M_P) \
			         $(ASOBJECTS_M_P) | tsort`

#
# Rules for compiling regular object files for libm.a
#
$(DPOBJECTS) $(KROBJECTS):
	$(CC) $(CFLAGS) -UlibM -c $< -o $@

$(SPOBJECTS):
	$(CC) $(CFLAGS) $(SPFLAGS) -UlibM -c $< -o $@

$(ASOBJECTS):
	$(M4) $(M4FLAGS) -UlibM $< | $(AS) $(ASFLAGS) -o $@

#
# Rules for compiling regular object files for libM.a
#
$(DPOBJECTS_M):
	$(CC) $(CFLAGS) -DlibM -c $< -o $@

$(SPOBJECTS_M):
	$(CC) $(CFLAGS) $(SPFLAGS) -DlibM -c $< -o $@

$(ASOBJECTS_M):
	$(M4) $(M4FLAGS) -DlibM $< | $(AS) $(ASFLAGS) -o $@

#
# Rules for compiling shared object files for libm.sl
#
$(DPOBJECTS_SL) $(KROBJECTS_SL):
	$(CC) +z $(CFLAGS) -UlibM -c $< -o $@

$(SPOBJECTS_SL):
	$(CC) +z $(CFLAGS) $(SPFLAGS) -UlibM -c $< -o $@

$(ASOBJECTS_SL) $(AS80OBJECTS_SL):
	$(M4) $(M4FLAGS) -UlibM -DPIC $< | $(AS) +z $(ASFLAGS) -o $@

#
# Rules for compiling shared object files for libM.sl
#
$(DPOBJECTS_M_SL):
	$(CC) +z $(CFLAGS) -DlibM -c $< -o $@

$(SPOBJECTS_M_SL):
	$(CC) +z $(CFLAGS) $(SPFLAGS) -DlibM -c $< -o $@

$(ASOBJECTS_M_SL) $(AS80OBJECTS_M_SL):
	$(M4) $(M4FLAGS) -DlibM -DPIC $< | $(AS) +z $(ASFLAGS) -o $@

#
# Rules for compiling profiled object files for libm_p.a
#
$(DPOBJECTS_P) $(KROBJECTS_P):
	$(CC) -p $(CFLAGS) -UlibM -c $< -o $@

$(SPOBJECTS_P):
	$(CC) -p $(CFLAGS) $(SPFLAGS) -UlibM -c $< -o $@

$(ASOBJECTS_P):
	$(M4) $(M4FLAGS) -UlibM -DPROFILE $< | $(AS) $(ASFLAGS) -o $@

#
# Rules for compiling profiled object files for libM_p.a
#
$(DPOBJECTS_M_P):
	$(CC) -p $(CFLAGS) -DlibM -c $< -o $@

$(SPOBJECTS_M_P):
	$(CC) -p $(CFLAGS) $(SPFLAGS) -DlibM -c $< -o $@

$(ASOBJECTS_M_P):
	$(M4) $(M4FLAGS) -DlibM -DPROFILE $< | $(AS) $(ASFLAGS) -o $@

#
# Cleanup rules
#
clean:
	@echo '\tRemoving *.o'
	@/bin/rm -f *.o

clobber: clean
	/bin/rm -f $(LIBm) $(LIBM) \
		   $(LIBm_SL) $(LIBM_SL) $(LIBm_PIC) $(LIBM_PIC) \
		   $(LIBm_P) $(LIBM_P)

#
# Clear SUFFIXES, then reset ordering so if a .c and .s source exist
# for a file, the .s will be compiled first
#
.SUFFIXES:
.SUFFIXES: .o .s .c _p.o _sl.o M.o M_p.o M_sl.o

.c_sl.o:
	@echo "Dummy rule to get make to make _sl.o files from .c files"

.s_sl.o:
	@echo "Dummy rule to get make to make _sl.o files from .s files"

.c_p.o:
	@echo "Dummy rule to get make to make _p.o files from .c files"

.s_p.o:
	@echo "Dummy rule to get make to make _p.o files from .s files"

.cM.o:
	@echo "Dummy rule to get make to make M.o files from .c files"

.sM.o:
	@echo "Dummy rule to get make to make M.o files from .s files"

.cM_sl.o:
	@echo "Dummy rule to get make to make M_sl.o files from .c files"

.sM_sl.o:
	@echo "Dummy rule to get make to make M_sl.o files from .s files"

.cM_p.o:
	@echo "Dummy rule to get make to make M_p.o files from .c files"

.sM_p.o:
	@echo "Dummy rule to get make to make M_p.o files from .s files"
# copy /lib/libm to the . relative fileset C-MIN
# change mode, owner, and group on /lib/libm
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
	mkdir -p ../../../Filesets.sl/C-MIN/lib
	! cp libM.a ../../../Filesets.sl/C-MIN/lib/libM.a
	! chmog 0444 bin bin ../../../Filesets.sl/C-MIN/lib/libM.a
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/lib
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libm to the . relative fileset CORE-SHLIBS
# change mode, owner, and group on /lib/libm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CORE-SHLIBS/lib
	! cp libM.sl ../../../Filesets.sl/CORE-SHLIBS/lib/libM.sl
	! chmog 0555 bin bin ../../../Filesets.sl/CORE-SHLIBS/lib/libM.sl
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/lib
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libm to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/libm
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	! cp libm.a ../../../Filesets.sl/BUILDENV/lib/libm.a
	! chmog 0444 bin bin ../../../Filesets.sl/BUILDENV/lib/libm.a
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libm to the . relative fileset C-MIN
# change mode, owner, and group on /lib/libm
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C-MIN/lib
	! cp libm.a ../../../Filesets.sl/C-MIN/lib/libm.a
	! chmog 0444 bin bin ../../../Filesets.sl/C-MIN/lib/libm.a
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/lib
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libm to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/libm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	! cp libm.sl ../../../Filesets.sl/BUILDENV/lib/libm.sl
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/lib/libm.sl
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# libm.sl to xterm300
	mkdir -p ../../../Filesets.sl/XTERM300/usr/xterm300/lib
	! cp libm.sl ../../../Filesets.sl/XTERM300/usr/xterm300/lib/libm.sl
	! chmog 0555 bin bin ../../../Filesets.sl/XTERM300/usr/xterm300/lib/libm.sl
	! chmog 0555 bin bin ../../../Filesets.sl/XTERM300/usr/xterm300/lib
	! chmog 0555 bin bin ../../../Filesets.sl/XTERM300/usr/xterm300
	! chmog 0555 bin bin ../../../Filesets.sl/XTERM300/usr
	! chmog 0555 bin bin ../../../Filesets.sl/XTERM300
	! chmog 0555 bin bin ../../../Filesets.sl
# copy /lib/libm to the . relative fileset CORE-SHLIBS
# change mode, owner, and group on /lib/libm
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/CORE-SHLIBS/lib
	! cp libm.sl ../../../Filesets.sl/CORE-SHLIBS/lib/libm.sl
	! chmog 0555 bin bin ../../../Filesets.sl/CORE-SHLIBS/lib/libm.sl
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/lib
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libm to the . relative fileset PROG-AUX
# change mode, owner, and group on /lib/libm
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/PROG-AUX/lib/libp
	! cp libM_p.a ../../../Filesets.sl/PROG-AUX/lib/libp/libM.a
	! chmog 0444 bin bin ../../../Filesets.sl/PROG-AUX/lib/libp/libM.a
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/lib/libp
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/lib/
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/libm to the . relative fileset PROG-AUX
# change mode, owner, and group on /lib/libm
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/PROG-AUX/lib/libp
	! cp libm_p.a ../../../Filesets.sl/PROG-AUX/lib/libp/libm.a
	! chmog 0444 bin bin ../../../Filesets.sl/PROG-AUX/lib/libp/libm.a
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/lib/libp
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/lib/
	! chmog 555 bin bin ../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../Filesets.sl/
