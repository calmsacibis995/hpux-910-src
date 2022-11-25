# $Source: /misc/source_product/9.10/commands.rcs/lib/libc/csu/makefile,v $
# $Revision: 70.6 $

# Common makefile for libc/csu

#
# Note: gcrt0.o and mcrt0.o are also built for the 800, but it uses a
#       special target "mgcrt0".  mcrt0.o and gcrt0.o depend on
#       different things depending on which architecture they're built
#       on.
#

# flags need for building the s800 version of syscalls.o (for crt0.o)
CPP=/lib/cpp
PREFIX=/lib/pcc_prefix.s
SED=    /bin/sed
SEDFLAGS= -e '/^[	]*$$/d' -e 's/^\./	\./' -e 's/; _/;/' -e 'y/; /\n	/' -e 's/ /	/'

NO_LIB_OBJS300 = do_crt0
NO_LIB_OBJS800 = do_crt0 crt1.o crtx.o
OBJS800 = _start.o environ.o
OBJS300 = _start.o environ.o
OBJS800_SL = $(OBJS800:.o=_sl.o)
OBJS300_SL = $(OBJS300:.o=_sl.o)
OBJS800_P = $(OBJS800:.o=_p.o)
OBJS300_P = $(OBJS300:.o=_p.o)
OBJS800_PIC = $(OBJS800:.o=_pic.o)
OBJS300_PIC = $(OBJS300:.o=_pic.o)

default:	archive

archive:
	@if hp9000s300; then				\
		no_lib_objs="$(NO_LIB_OBJS300)";	\
		assembler=/bin/as20;			\
		objs="$(OBJS300)";			\
	elif hp9000s800; then				\
		no_lib_objs="$(NO_LIB_OBJS800)"		\
		assembler=/bin/as;			\
		objs="$(OBJS800)";			\
	fi;						\
	$(MAKE) NO_LIB_OBJS="$$no_lib_objs" OBJS="$$objs" \
	AS=$$assembler DIR=tmp csu

shared:
	@if hp9000s300; then				\
		no_lib_objs="nothing";			\
		assembler=/bin/as20;			\
		objs="$(OBJS300_SL)";			\
	elif hp9000s800; then				\
		no_lib_objs="nothing";			\
		assembler=/bin/as;			\
		objs="$(OBJS800_SL) crt1_sl.o";		\
	fi;						\
	$(MAKE) NO_LIB_OBJS="$$no_lib_objs" OBJS="$$objs" \
	AS=$$assembler DIR=tmp_sl csu

profile:
	@if hp9000s800; then				\
		no_lib_objs="$(NO_LIB_OBJS800)";	\
		objs="$(OBJS800_P)";			\
	elif hp9000s300; then				\
		no_lib_objs="nothing";			\
		objs="$(OBJS300_P)";			\
	fi;						\
	$(MAKE) NO_LIB_OBJS="$$no_lib_objs" OBJS="$$objs" DIR=tmp_p csu

archive_pic:
	@if hp9000s300; then				\
		no_lib_objs="nothing";			\
		assembler=/bin/as20;			\
		objs="$(OBJS300_PIC)";			\
	elif hp9000s800; then				\
		no_lib_objs="nothing";			\
		assembler=/bin/as;			\
		objs="$(OBJS800_PIC)";			\
	fi;						\
	$(MAKE) NO_LIB_OBJS="$$no_lib_objs" OBJS="$$objs"	 \
	AS=$$assembler DIR=tmp_pic csu

#
# The following two targets keep the crt objects from being
# built with the PIC option.
#

nothing:

nopic:
	$(MAKE) ASFLAGS="" ASFLG_P="" $(NO_LIB_OBJS)

csu: $(OBJS) nopic
	@echo "\tUsed AS = $(AS)";		\
	if [ ! -d ../$(DIR) ]; then		\
	    mkdir ../$(DIR) >/dev/null 2>&1;	\
	fi;					\
	exit 0
	@if [ -n "$(OBJS)" ]; then				\
	    echo "\tLinking objects to ../$(DIR)...";		\
	    ln $(OBJS) ../$(DIR);				\
	else							\
	    echo "\tNo objects from libc/csu moved to ../$(DIR)"; \
	fi
	touch ../$(DIR)/OK.$@

#
# This target builds the various flavors of crt0
#
# The +OL on the mapdld.c command line produces long branches.
# This will speed up link times.  Starting with the C/Ucode compiler
# at HP-UX 9.0, the supported option "+b0" should be used. - Ken Sumrall
do_crt0:
	if hp9000s300; then				\
	    $(CC) -c crt0.c;				\
	    $(CC) -c -DMCRT0 crt0.c -o mcrt0.o;		\
	    $(CC) -c -DGCRT0 crt0.c -o gcrt0.o;		\
	else						\
	    ${CPP} ${CPPFLAGS} syscalls.c   		\
	      | ${SED} ${SEDFLAGS} 			\
       	      | cat ${PREFIX} - | ${AS} -o syscalls.o;	\
	    ${CC} ${CFLAGS} -c crt0.s ;		\
	    ${CC} ${CFLAGS} +OL -c mapdld.c; 		\
	    $(LD) -r crt0.o mapdld.o syscalls.o -h mmap -h read -h write -h lseek -h __map_dld -h open -h close -h __exit -o crt.o;	\
	    $(CC) ${CFLAGS} -DMCRT0 -c crt0.s ;		\
	    $(CC) ${CFLAGS} -DMCRT0 -DUNIQUEMONS -c monx.c;	\
	    $(CC) ${CFLAGS} -DMCRT0 -DUNIQUEGMONS -c gmon.c;	\
	    $(LD) -r crt0.o mapdld.o syscalls.o monx.o -h mmap -h read -h write -h lseek -h __map_dld -h open -h close -h __exit -o mcrt0.o;\
	    $(LD) -r crt0.o mapdld.o syscalls.o gmon.o -h mmap -h read -h write -h lseek -h __map_dld -h open -h close -h __exit -o gcrt0.o;\
	    $(CC) ${CFLAGS} -DICRT0 -c crt0.s ;         \
	    $(CC) ${CFLAGS} -c -Aa -D_POSIX_SOURCE pbo.c;    \
	    $(LD) -r crt0.o mapdld.o syscalls.o pbo.o -h mmap -h read -h write -h lseek -h __map_dld -h open -h close -h __exit -o icrt0.o;\
	    mv -f crt.o crt0.o;				\
	fi

# This target is only for the s800 to support system calls needed for 
# shared libs

# This target is only for the s800, and must not be compiled with $(CC).
crt1.o crt1_sl.o:
	$(AS) < crt1.s > $@

clean:
	/bin/rm -f $(NO_LIB_OBJS300) $(NO_LIB_OBJS800) $(OBJS800) \
		$(OBJS300) $(OBJS800_SL) $(OBJS300_SL) $(OBJS800_P) \
		$(OBJS300_P) $(OBJS800_PIC) $(OBJS300_PIC)

#
# Using "$(AS)" does not allow a profiling option, so we access the
# assembler through $(CC) and use the same flags we use for assembling,
# along with the profiling option to cc.
# The s800 assembly source also has C pre-processor directives in it, so
# we need to run it through cc for that reason as well.
#
.s_p.o:
	$(CC) -Wa,$(ASFLG_P) -ta,$(AS) $(PRFLAGS) -c $< -o $@
.s.o:
	if hp9000s300; then			\
		$(M4) $(M4FLAGS) $< | $(AS) $(ASFLAGS) -o $@;   \
	elif hp9000s800; then			\
		$(CC) $(ASFLAGS) -c $<;		\
	fi


.s_sl.o:
	if hp9000s300; then			\
		$(M4) $(M4FLAGS) $< | $(AS) $(ASFLAGS) -o $@;   \
	elif hp9000s800; then			\
		$(CC) $(ASFLAGS) -c $<;		\
	fi

.s_pic.o:
	if hp9000s300; then			\
		$(M4) $(M4FLAGS) $< | $(AS) $(ASFLAGS) -o $@;   \
	elif hp9000s800; then			\
		$(CC) $(ASFLAGS) -c $<;		\
	fi

#
# Rules so that make will know how to create object files named foo_p.o,
# foo_sl.o, and foo_pic.o from a .c file named foo.c
#
.c_p.o:
	$(CC) $(CFLAGS) $(INCLUDES) $(PRFLAGS) -c $< -o $@

.c_sl.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.c_pic.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

#
# Clear SUFFIXES, then reset the defaults so that if a .s and a .c exist
# for a file, the .s will be compiled first
#
.SUFFIXES:
.SUFFIXES: .o .s .c _p.o _sl.o _pic.o
