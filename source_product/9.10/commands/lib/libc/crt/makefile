# $Source: /misc/source_product/9.10/commands.rcs/lib/libc/crt/makefile,v $
# $Revision: 66.12 $

# Common makefile for libc/crt subdirectory


# NOTE: QUAD_C_800 and QUAD_C_OPT_800 are set to dummy1.o and dummy2.o
#       if we are on a S300 machine.
#       This is done because eight objects are defined both in OBJS300
#       and QUAD_C_800/QUAD_C_OPT_800.  To compile correctly on the
#       S300, the rules for QUAD_C_800 and QUAD_C_OPT_800 are
#       effectively nullified so that default rules (the rules needed
#	for S300 builds) are used.

CPP = /lib/cpp

OBJS300 = cerror.o aldiv.o alrem.o auldiv.o aulmul.o aulrem.o S300_ldiv.o \
	  lrem.o uldiv.o ulmul.o ulrem.o fpasmd.o fpsupport.o fpshorts.o \
	  fpbcdrel.o fneg.o frexp.o rellnt.o rnd.o mcount.o flag_68881.o \
	  fpunsign.o U_Qfadd.o U_Qfcmp.o U_Qfdiv.o U_Qfmpy.o U_Qfsub.o \
          U_Qfneg.o U_Qflogneg.o qfadd.o qfcmp.o qfdiv.o qfmpy.o qfsub.o \
          U_Qfcnvff.o U_Qfcnvfxt.o U_Qfcnvxf.o qfcnvff.o qfcnvfxt.o qfcnvxf.o


OBJS800 = 

QUAD_ASSEM_800 = U_sqfcnvxf.o U_qsfcnvxf.o U_qqfcnvxf.o U_qdfcnvxf.o \
	U_dqfcnvxf.o U_sqfcnvft.o U_qsfcnvft.o U_qqfcnvft.o U_qdfcnvft.o \
	U_dqfcnvft.o U_sqfcnvfx.o U_qsfcnvfx.o U_qqfcnvfx.o U_qdfcnvfx.o \
	U_dqfcnvfx.o U_sqfcnvff.o U_qsfcnvff.o U_qdfcnvff.o U_dqfcnvff.o \
	U_qfcpy.o U_qfsub.o U_qfsqrt.o U_qfrnd.o U_qfrem.o U_qfmpy.o U_qfdiv.o \
	U_qfcmp.o U_qfadd.o U_qfabs.o fpasm.o

QUAD_C_800 = U_Qfsub.o U_Qfsqrt.o U_Qfrnd.o U_Qfrem.o U_Qfmpy.o U_Qfdiv.o \
	U_Qfcmp.o U_Qfmin.o U_Qfmax.o U_Qfadd.o U_Qfabs.o

QUAD_C_OPT_800 = U_Qfcnvxf.o U_Qfcnvfxt.o U_Qfcnvfx.o U_Qfcnvff.o

OBJS300_SL = $(OBJS300:.o=_sl.o)
OBJS800_SL = $(OBJS800:.o=_sl.o)
QUAD_ASSEM_800_SL = $(QUAD_ASSEM_800:.o=_sl.o)
QUAD_C_800_SL = $(QUAD_C_800:.o=_sl.o)
QUAD_C_OPT_800_SL = $(QUAD_C_OPT_800:.o=_sl.o)

OBJS300_P = $(OBJS300:.o=_p.o)
OBJS800_P = $(OBJS800:.o=_p.o)
QUAD_ASSEM_800_P = $(QUAD_ASSEM_800:.o=_p.o)
QUAD_C_800_P = $(QUAD_C_800:.o=_p.o)
QUAD_C_OPT_800_P = $(QUAD_C_OPT_800:.o=_p.o)

OBJS300_PIC = $(OBJS300:.o=_pic.o)
OBJS800_PIC = $(OBJS800:.o=_pic.o)
QUAD_ASSEM_800_PIC = $(QUAD_ASSEM_800:.o=_pic.o)
QUAD_C_800_PIC = $(QUAD_C_800:.o=_pic.o)
QUAD_C_OPT_800_PIC = $(QUAD_C_OPT_800:.o=_pic.o)

default:	archive

archive:
	@if hp9000s300; then					\
		objs="$(OBJS300)";				\
		qc8="dummy1.o";					\
		qco8="dummy2.o";				\
	elif hp9000s800; then					\
		objs="$(OBJS800) $(QUAD_ASSEM_800)		\
		$(QUAD_C_800) $(QUAD_C_OPT_800)";		\
		qc8="$(QUAD_C_800)";				\
		qco8="$(QUAD_C_OPT_800)";			\
	fi;							\
	$(MAKE) OBJS="$$objs" QUAD_C_800="$$qc8"	\
		QUAD_C_OPT_800="$$qco8" DIR="tmp" crt

shared:
	@if hp9000s300; then					\
		objs="$(OBJS300_SL)";				\
		qc8="dummy1.o";					\
		qco8="dummy2.o";				\
	elif hp9000s800; then					\
		objs="$(OBJS800_SL) $(QUAD_ASSEM_800_SL)	\
		$(QUAD_C_800_SL) $(QUAD_C_OPT_800_SL)";		\
		qc8="$(QUAD_C_800)";				\
		qco8="$(QUAD_C_OPT_800)";			\
	fi;							\
	$(MAKE) OBJS="$$objs" QUAD_C_800="$$qc8"	\
		QUAD_C_OPT_800="$$qco8" DIR="tmp_sl" crt

profile:
	@if hp9000s300; then					\
		objs="$(OBJS300_P)";				\
		qc8="dummy1.o";					\
		qco8="dummy2.o";				\
	elif hp9000s800; then					\
		objs="$(OBJS800_P) $(QUAD_ASSEM_800_P)		\
		$(QUAD_C_800_P) $(QUAD_C_OPT_800_P)";		\
		qc8="$(QUAD_C_800)";				\
		qco8="$(QUAD_C_OPT_800)";			\
	fi;							\
	$(MAKE) OBJS="$$objs" QUAD_C_800="$$qc8"	\
		QUAD_C_OPT_800="$$qco8" DIR="tmp_p" crt

archive_pic:
	@if hp9000s300; then					\
		objs="$(OBJS300_PIC)";				\
		qc8="dummy1.o";					\
		qco8="dummy2.o";				\
	elif hp9000s800; then					\
		objs="$(OBJS800_PIC) $(QUAD_ASSEM_800_PIC)	\
		$(QUAD_C_800_PIC) $(QUAD_C_OPT_800_PIC)";	\
		qc8="$(QUAD_C_800)";				\
		qco8="$(QUAD_C_OPT_800)";			\
	fi;							\
	$(MAKE) OBJS="$$objs" QUAD_C_800="$$qc8"	\
		QUAD_C_OPT_800="$$qco8" DIR="tmp_pic" crt

crt: $(OBJS)
	@echo "\tLinking objects to ../$(DIR)..."
	@if [ ! -d ../$(DIR) ]; then		\
	    mkdir ../$(DIR) >/dev/null 2>&1;	\
	fi;					\
	exit 0
	@ln $(OBJS) ../$(DIR)
	touch ../$(DIR)/OK.$@

$(QUAD_ASSEM_800):
	cc $(CFLAGS) -c $< -o $@

$(QUAD_ASSEM_800_SL):
	cc $(CFLAGS) -c $< -o $@

$(QUAD_ASSEM_800_P):
	cc $(CFLAGS) $(PRFLAGS) -c $< -o $@

$(QUAD_ASSEM_800_PIC):
	cc $(CFLAGS) -c $< -o $@

$(QUAD_C_800): quadfp.h

$(QUAD_C_800_P): quadfp.h

$(QUAD_C_800_SL): quadfp.h

$(QUAD_C_800_PIC): quadfp.h

$(QUAD_C_OPT_800): quadfp.h
	cc $(CFLAGS) -c +r $< -o $@

$(QUAD_C_OPT_800_SL): quadfp.h
	cc $(CFLAGS) -c +r $< -o $@

$(QUAD_C_OPT_800_P):
	cc $(CFLAGS) $(PRFLAGS) -c +r $< -o $@

$(QUAD_C_OPT_800_PIC): quadfp.h
	cc $(CFLAGS) -c +r $< -o $@

clean:
	/bin/rm -f $(OBJS300) $(OBJS300_P) $(OBJS300_SL) $(OBJS300_PIC) \
	$(OBJS800) $(OBJS800_P) $(OBJS300_SL) $(OBJS800_PIC) \
	$(QUAD_ASSEM_800) $(QUAD_ASSEM_800_P) $(QUAD_ASSEM_800_SL) \
	$(QUAD_ASSEM_800_PIC) \
	$(QUAD_C_800) $(QUAD_C_800_P) $(QUAD_C_800_SL) $(QUAD_C_800_PIC) \
	$(QUAD_C_OPT_800) $(QUAD_C_OPT_800_P) \
	$(QUAD_C_OPT_800_SL) $(QUAD_C_OPT_800_PIC)

#
# Define rules so that make knows how to compile assembly sources for
# both profiled and non-profiled objects, for both s300 and s800.
# .s_p.o rule indicates how to make a profiled object named file_p.o,
# to distinguish it from the regular file.o
#
.c_p.o:
	$(CC) $(CFLAGS) $(PRFLAGS) -c $< -o $@
.c_sl.o:
	$(CC) $(CFLAGS) -c $< -o $@
.c_pic.o:
	$(CC) $(CFLAGS) -c $< -o $@
.s.o:
	if hp9000s300; then					\
		$(M4) $(M4FLAGS) $< | $(AS) $(ASFLAGS) -o $@;	\
	elif hp9000s800; then					\
		$(CPP) $(CPPFLAGS) $< | $(AS) $(ASFLAGS) -o $@;	\
	fi;

.s_p.o:
	if hp9000s300; then					\
		$(M4) $(M4FLAGS) $< | $(AS) $(ASFLG_P) -o $@;	\
	elif hp9000s800; then					\
		$(CPP) $(CPPFLG_P) $< | $(AS) $(ASFLG_P) -o $@;	\
	fi;
	
.s_sl.o:
	if hp9000s300; then					\
		$(M4) $(M4FLAGS) $< | $(AS) $(ASFLAGS) -o $@;	\
	elif hp9000s800; then					\
		$(CPP) $(CPPFLAGS) $< | $(AS) $(ASFLAGS) -o $@;	\
	fi;
	
.s_pic.o:
	if hp9000s300; then					\
		$(M4) $(M4FLAGS) $< | $(AS) $(ASFLAGS) -o $@;	\
	elif hp9000s800; then					\
		$(CPP) $(CPPFLAGS) $< | $(AS) $(ASFLAGS) -o $@;	\
	fi;
	
#
# Clear SUFFIXES, then reset the default suffixes so that if a .c and
# a .s both exist for some file, the .s will be compiled first
#
.SUFFIXES:
.SUFFIXES: .o .s .c _p.o _sl.o _pic.o
