# @(#) $Revision: 72.2 $
#
# Common makefile for libc/sys subdirectory
#

CFLG800 = -DFLOAT
CFLG700 = -DFLOAT
CPP_FLGS = -DAS -DLOCORE
SEDFLAGS= -e '/^[ 	]*$$/d' -e 's/\\,/,/' -e 's/^\./	\./' -e 's/; _/;/' -e 'y/; /\n	/' -e 's/ /	/'

OBJS800 = \
		_setjmp.o	\
		brk.o		\
		execl.o		\
		execle.o	\
		execlp.o	\
		execv.o		\
		hostnm.o	\
		mkfifo.o	\
		privgrp.o	\
		sbrk.o		\
		setjmp.o	\
		setuname.o	\
		signal.o	\
		sigsetjmp.o	\
		sigspace.o	\
		sigvector.o	\
		signalvec.o	\
		syscall.o	\
		ulimit.o	\
		uname.o		\
		ustat.o

OBJS300 = \
		brk.o		\
		execl.o 	\
		execle.o 	\
		execv.o 	\
		hostnm.o	\
		mkfifo.o 	\
		prealloc.o 	\
		privgrp.o 	\
		sbrk.o		\
		setuname.o 	\
		signal.o 	\
		sigspace.o	\
		ulimit.o

#
# This routine is currently supported for s300 and s700 only.
#
OBJS300_700 =	msemlock.o	\
		mseminit.o

OBJS800_P   = $(OBJS800:.o=_p.o)
OBJS300_P   = $(OBJS300:.o=_p.o)
OBJS300_700_P   = $(OBJS300_700:.o=_p.o)

OBJS800_SL  = $(OBJS800:.o=_sl.o)
OBJS300_SL  = $(OBJS300:.o=_sl.o)
OBJS300_700_SL  = $(OBJS300_700:.o=_sl.o)

OBJS800_PIC = $(OBJS800:.o=_pic.o)
OBJS300_PIC = $(OBJS300:.o=_pic.o)
OBJS300_700_PIC = $(OBJS300_700:.o=_pic.o)

default:	archive

archive:
	@if hp9000s300; then				\
	    objs="$(OBJS300) $(OBJS300_700)";		\
	    cflags="$(CFLAGS)";				\
	elif hp9000s700; then				\
	    objs="$(OBJS800) $(OBJS300_700)";		\
	    cflags="$(CFLAGS) $(CFLG700)";		\
	elif hp9000s800; then				\
	    objs="$(OBJS800)";				\
	    cflags="$(CFLAGS) $(CFLG800)";		\
	fi;						\
	$(MAKE) CFLAGS="$$cflags" OBJS="$$objs" DIR=tmp sys

shared:
	@if hp9000s300; then				\
	    objs="$(OBJS300_SL) $(OBJS300_700_SL)";	\
	    cflags="$(CFLAGS)";				\
	elif hp9000s700; then				\
	    objs="$(OBJS800_SL) $(OBJS300_700_SL)";	\
	    cflags="$(CFLAGS) $(CFLG700)";		\
	elif hp9000s800; then				\
	    objs="$(OBJS800_SL)";			\
	    cflags="$(CFLAGS) $(CFLG800)";		\
	fi;						\
	$(MAKE) CFLAGS="$$cflags" OBJS="$$objs" DIR=tmp_sl sys

profile:
	@if hp9000s300; then				\
	    objs="$(OBJS300_P) $(OBJS300_700_P)";	\
	    cflags="$(CFLAGS)";				\
	elif hp9000s700; then				\
	    objs="$(OBJS800_P) $(OBJS300_700_P)";	\
	    cflags="$(CFLAGS) $(CFLG700)";		\
	elif hp9000s800; then				\
	    objs="$(OBJS800_P)";			\
	    cflags="$(CFLAGS) $(CFLG800)";		\
	fi;						\
	$(MAKE) CFLAGS="$$cflags" OBJS="$$objs sbrk_noprof.o" DIR=tmp_p sys

archive_pic:
	@if hp9000s300; then				\
	    objs="$(OBJS300_PIC) $(OBJS300_700_PIC)";	\
	    cflags="$(CFLAGS)";				\
	elif hp9000s700; then				\
	    objs="$(OBJS800_PIC) $(OBJS300_700_PIC)";	\
	    cflags="$(CFLAGS) $(CFLG700)";		\
	elif hp9000s800; then				\
	    objs="$(OBJS800_PIC)";			\
	    cflags="$(CFLAGS) $(CFLG800)";		\
	fi;						\
	$(MAKE) CFLAGS="$$cflags" OBJS="$$objs" DIR=tmp_pic sys

sys: $(OBJS)
	@echo "\tLinking objects to ../$(DIR)..."
	@if [ ! -d ../$(DIR) ]; then		\
	    mkdir ../$(DIR) >/dev/null 2>&1;	\
	fi;					\
	exit 0
	@ln $(OBJS) ../$(DIR)
	touch ../$(DIR)/OK.$@

sbrk_noprof.o: sbrk.c
	$(CC) -c -DNON_PROFILE sbrk.c -o $@

.s.o:
	if hp9000s300; then					 \
	     $(M4) $(M4FLAGS) $< | $(AS) $(ASFLAGS) > $@;	 \
	elif hp9000s800; then					 \
	    $(CC) -E $(CPPFLAGS) $(CPP_FLGS) $< | $(SED) $(SEDFLAGS) \
	    | cat $(PREFIX) - | $(AS) $(ASFLAGS) > $@;		 \
	fi

.s_sl.o:
	if hp9000s300; then					 \
	     $(M4) $(M4FLAGS) $< | $(AS) $(ASFLAGS) > $@;	 \
	elif hp9000s800; then					 \
	    $(CC) -E $(CPPFLAGS) $(CPP_FLGS) $< | $(SED) $(SEDFLAGS) \
	    | cat $(PREFIX) - | $(AS) $(ASFLAGS) > $@;		 \
	fi

.s_pic.o:
	if hp9000s300; then					 \
	     $(M4) $(M4FLAGS) $< | $(AS) $(ASFLAGS) > $@;	 \
	elif hp9000s800; then					 \
	    $(CC) -E $(CPPFLAGS) $(CPP_FLGS) $< | $(SED) $(SEDFLAGS) \
	    | cat $(PREFIX) - | $(AS) $(ASFLAGS) > $@;		 \
	fi

.s_p.o:
	if hp9000s300; then					 \
	    $(M4) $(M4FLAGS) $< | $(AS) $(ASFLG_P) > $@;	 \
	elif hp9000s800; then					 \
	    $(CC) -E $(CPPFLG_P) $(CPP_FLGS) $< | $(SED) $(SEDFLAGS) \
	    | cat $(PREFIX) - | $(AS) $(ASFLG_P) > $@;		 \
	fi

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.c_sl.o:
	$(CC) $(CFLAGS) -c $< -o $@

.c_pic.o:
	$(CC) $(CFLAGS) -c $< -o $@

.c_p.o:
	$(CC) $(CFLAGS) $(PRFLAGS) -c $< -o $@

clean:
	rm -f *.o

clobber:
	@echo "Clobbering is done by the top-level libc makefile"

#
# Clear SUFFIXES, then reset ordering so if a .s and a .c source exist
# for a file, the .s will be compiled first
#
.SUFFIXES:
.SUFFIXES: .o .s .c _p.o _sl.o _pic.o
