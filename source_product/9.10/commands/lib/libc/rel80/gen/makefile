# $Source: /misc/source_product/9.10/commands.rcs/lib/libc/rel80/gen/makefile,v $
# $Revision: 70.6 $

# Common makefile for libc.gen

GENCAT=/usr/bin/gencat
FINDMSG=/usr/bin/findmsg
SEDFLAGS= -e '/^[ 	]*$$/d' -e 's/\\,/,/' -e 's/\\,/,/' -e 's/^\./	\./' -e 's/; _/;/' -e 'y/; /\n	/' -e 's/ /	/'
INCLUDE_80 = ../h
CFLAGS_80 = $(CFLAGS) -I$(INCLUDE_80)

COMOBJS = \
	_errlocale80.o

ACL_OBJS = 

AUD_OBJS = 

DISKLESS_OBJS = 

FSS_OBJS = 

AES_OBJS = 

GETMOUNT_OBJS = 

NLS_OBJS = \
	getlocale80.o	\
	nl_langinf80.o	\
	setlocale80.o \
	nl_string80.o \
	_collxfrm80.o \
	nl_strcmp80.o \
	strcoll80.o \
	strxfrm80.o

OBJS300 = 

OBJS800 = 

ASMOBJS300 =

ASMOBJS800 =

BACK_COMPATIBLE = \
	regex80.o \
	rew80.o

COMOBJS_P         = $(COMOBJS:.o=_p.o)
ACL_OBJS_P        = $(ACL_OBJS:.o=_p.o)
AUD_OBJS_P        = $(AUD_OBJS:.o=_p.o)
DISKLESS_OBJS_P   = $(DISKLESS_OBJS:.o=_p.o)
FSS_OBJS_P        = $(FSS_OBJS:.o=_p.o)
AES_OBJS_P        = $(AES_OBJS:.o=_p.o)
GETMOUNT_OBJS_P   = $(GETMOUNT_OBJS:.o=_p.o)
NLS_OBJS_P        = $(NLS_OBJS:.o=_p.o)
OBJS300_P         = $(OBJS300:.o=_p.o)
OBJS800_P         = $(OBJS800:.o=_p.o)
ASMOBJS300_P      = $(ASMOBJS300:.o=_p.o)
ASMOBJS800_P      = $(ASMOBJS800:.o=_p.o)

COMOBJS_SL        = $(COMOBJS:.o=_sl.o)
ACL_OBJS_SL       = $(ACL_OBJS:.o=_sl.o)
AUD_OBJS_SL       = $(AUD_OBJS:.o=_sl.o)
DISKLESS_OBJS_SL  = $(DISKLESS_OBJS:.o=_sl.o)
FSS_OBJS_SL       = $(FSS_OBJS:.o=_sl.o)
AES_OBJS_SL       = $(AES_OBJS:.o=_sl.o)
GETMOUNT_OBJS_SL  = $(GETMOUNT_OBJS:.o=_sl.o)
NLS_OBJS_SL       = $(NLS_OBJS:.o=_sl.o)
OBJS300_SL        = $(OBJS300:.o=_sl.o)
OBJS800_SL        = $(OBJS800:.o=_sl.o)
ASMOBJS300_SL     = $(ASMOBJS300:.o=_sl.o)
ASMOBJS800_SL     = $(ASMOBJS800:.o=_sl.o)
BACK_COMPATIBLE_SL= $(BACK_COMPATIBLE:.o=_sl.o)

COMOBJS_PIC       = $(COMOBJS:.o=_pic.o)
ACL_OBJS_PIC      = $(ACL_OBJS:.o=_pic.o)
AUD_OBJS_PIC      = $(AUD_OBJS:.o=_pic.o)
DISKLESS_OBJS_PIC = $(DISKLESS_OBJS:.o=_pic.o)
FSS_OBJS_PIC      = $(FSS_OBJS:.o=_pic.o)
AES_OBJS_PIC      = $(AES_OBJS:.o=_pic.o)
GETMOUNT_OBJS_PIC = $(GETMOUNT_OBJS:.o=_pic.o)
NLS_OBJS_PIC      = $(NLS_OBJS:.o=_pic.o)
OBJS300_PIC       = $(OBJS300:.o=_pic.o)
OBJS800_PIC       = $(OBJS800:.o=_pic.o)
ASMOBJS300_PIC    = $(ASMOBJS300:.o=_pic.o)
ASMOBJS800_PIC    = $(ASMOBJS800:.o=_pic.o)

NPARTS=8
PART=all

default:	archive

archive:
#	@if hp9000s300; then					\
#	    objs="$(COMOBJS) $(OBJS300) $(ASMOBJS300)";		\
#	elif hp9000s800; then					\
#	    objs="$(COMOBJS) $(OBJS800) $(ASMOBJS800)";		\
#	fi;							\
#	ifdef ACLS      && objs="$$objs $(ACL_OBJS)";		\
#	ifdef AUDIT     && objs="$$objs $(AUD_OBJS)";		\
#	ifdef DISKLESS  && objs="$$objs $(DISKLESS_OBJS)";	\
#	ifdef FSS       && objs="$$objs $(FSS_OBJS)";		\
#	ifdef AES       && objs="$$objs $(AES_OBJS)";		\
#	ifdef GETMOUNT  && objs="$$objs $(GETMOUNT_OBJS)";	\
#	ifdef NLS       && objs="$$objs $(NLS_OBJS)";		\
#	$(MAKE) OBJS="$$objs" PART="$(PART)" DIR=tmp gen

shared:
	@if hp9000s300; then					\
	    objs="$(COMOBJS_SL) $(OBJS300_SL) $(ASMOBJS300_SL) $(BACK_COMPATIBLE_SL)";\
	elif hp9000s800; then					\
	    objs="$(COMOBJS_SL) $(OBJS800_SL) $(ASMOBJS800_SL) $(BACK_COMPATIBLE_SL)";\
	fi;							\
	ifdef ACLS      && objs="$$objs $(ACL_OBJS_SL)";	\
	ifdef AUDIT     && objs="$$objs $(AUD_OBJS_SL)";	\
	ifdef DISKLESS  && objs="$$objs $(DISKLESS_OBJS_SL)";	\
	ifdef FSS       && objs="$$objs $(FSS_OBJS_SL)";	\
	ifdef AES       && objs="$$objs $(AES_OBJS_SL)";	\
	ifdef GETMOUNT  && objs="$$objs $(GETMOUNT_OBJS_SL)";	\
	ifdef NLS       && objs="$$objs $(NLS_OBJS_SL)";	\
	rm -f *.o;						\
	$(MAKE) OBJS="$$objs" PART="$(PART)" DIR=tmp_sl gen

profile:
#	@if hp9000s300; then					\
#	    objs="$(COMOBJS_P) $(OBJS300_P) $(ASMOBJS300_P)";	\
#	elif hp9000s800; then					\
#	    objs="$(COMOBJS_P) $(OBJS800_P) $(ASMOBJS800_P)";	\
#	fi;							\
#	ifdef ACLS      && objs="$$objs $(ACL_OBJS_P)";		\
#	ifdef AUDIT     && objs="$$objs $(AUD_OBJS_P)";		\
#	ifdef DISKLESS  && objs="$$objs $(DISKLESS_OBJS_P)";	\
#	ifdef FSS       && objs="$$objs $(FSS_OBJS_P)";		\
#	ifdef AES       && objs="$$objs $(AES_OBJS_P)";		\
#	ifdef GETMOUNT  && objs="$$objs $(GETMOUNT_OBJS_P)";	\
#	ifdef NLS       && objs="$$objs $(NLS_OBJS_P)";		\
#	$(MAKE) OBJS="$$objs" PART="$(PART)" DIR=tmp_p gen

archive_pic:
#	@if hp9000s300; then					    \
#	    objs="$(COMOBJS_PIC) $(OBJS300_PIC) $(ASMOBJS300_PIC)"; \
#	elif hp9000s800; then					    \
#	    objs="$(COMOBJS_PIC) $(OBJS800_PIC) $(ASMOBJS800_PIC)"; \
#	fi;							    \
#	ifdef ACLS      && objs="$$objs $(ACL_OBJS_PIC)";	    \
#	ifdef AUDIT     && objs="$$objs $(AUD_OBJS_PIC)";	    \
#	ifdef DISKLESS  && objs="$$objs $(DISKLESS_OBJS_PIC)";	    \
#	ifdef FSS       && objs="$$objs $(FSS_OBJS_PIC)";	    \
#	ifdef AES       && objs="$$objs $(AES_OBJS_PIC)";	    \
#	ifdef GETMOUNT  && objs="$$objs $(GETMOUNT_OBJS_PIC)";	    \
#	ifdef NLS       && objs="$$objs $(NLS_OBJS_PIC)";	    \
#	$(MAKE) OBJS="$$objs" PART="$(PART)" DIR=tmp_pic gen


# gen -- Based on the value of $(PART), either just call do_gen (PART
#        is "all") or split the object list into $(NPART) pieces and
#        call d_gen with the PART piece of the entire object list.
#
#        The object list is split into NPART pieces by placing the
#        file names into a file [one file per line] and then using
#        split to split the file into the correct number of pieces.
#        The appropriate file is then chosen and the object file names
#        are read from it to be passed to "do_gen".
#
#
#        If PART is "all" or 1, we also create the message catalog for
#        perror().
#
#        NPARTS can be from 1 to 9.
#        
gen:
	@if [ "$(PART)" = "all" ]; then				\
	    objs="$(OBJS)";					\
	else							\
	    tmp=olst$$$$;					\
	    /bin/rm -f $$tmp;					\
	    for i in $(OBJS); do				\
		echo $$i;					\
	    done >$$tmp;					\
	    lines=`wc -l <$$tmp`;				\
	    n=`expr \( $$lines + $(NPARTS) - 1 \) / $(NPARTS)`;	\
	    split -$$n $$tmp $$tmp;				\
	    suffix=a`echo $(PART) | tr 123456789 abcdefghi`;	\
	    objs=`cat $$tmp$$suffix`;				\
	    objs=`echo $$objs`;					\
	    /bin/rm -f $$tmp*;					\
	fi;							\
	$(MAKE) OBJS="$$objs" PART=$(PART) DIR=$(DIR) do_gen;
#        if [ "$(PART)" = "all" -o "$(PART)" = "1" ]; then       \
#            echo "\tCreating perror.cat";                       \
#            echo '#define NL_SETN 1' >temp$$$$.c;               \
#            $(CC) -C -E errlst.c >>temp$$$$.c;                  \
#            $(FINDMSG) temp$$$$.c > perror.msg;                 \
#            $(GENCAT) perror.cat perror.msg;                    \
#            echo "\tCreating getopt_lib.cat";                   \
#            $(FINDMSG) getopt.c > getopt_lib.msg;               \
#            $(GENCAT) getopt_lib.cat getopt_lib.msg;            \
#            echo "\tCreating regcomp.cat";                      \
#            $(FINDMSG) regex.c > regcomp.msg;                   \
#            $(GENCAT) regcomp.cat regcomp.msg;                  \
#            rm -f temp$$$$.c;                                   \
#            if [ "$(PART)" = "all" ]; then                      \
#                touch ../$(DIR)/OK.gen;                         \
#            else                                                \
#                touch ../$(DIR)/OK.gen$(PART);                  \
#            fi;                                                 \
#        fi

#
# do_gen -- do the real work.  If PART is not all or 1 we also
#           touch a flag file to indicate that our part was
#           sucessful.  If part is all or 1, the "gen" target
#           will create the appropriate flag file.
#
do_gen: $(OBJS)
	@echo "\tLinking objects to ../../$(DIR)...";	\
	ln $(OBJS) ../../$(DIR);			\
	touch ../../tmp_sl/OK.rel80;			\
	if [ $(PART) != "all" -a "$(PART)" != 1 ]; then	\
	    touch ../../$(DIR)/OK.gen$(PART);		\
	fi

clean:
	@echo "\tCleaning gen..."
	@/bin/rm -f $(COMOBJS) $(OBJS300) $(OBJS800)		\
		    $(ASMOBJS300) $(ASMOBJS800)
	@/bin/rm -f $(COMOBJS_P) $(OBJS300_P) $(OBJS800_P)	\
		    $(ASMOBJS300_P) $(ASMOBJS800_P)
	@/bin/rm -f $(COMOBJS_SL) $(OBJS300_SL) $(OBJS800_SL)	\
		    $(ASMOBJS300_SL) $(ASMOBJS800_SL)
	@/bin/rm -f $(COMOBJS_PIC) $(OBJS300_PIC) $(OBJS800_PIC)\
		    $(ASMOBJS300_PIC) $(ASMOBJS800_PIC)
	@/bin/rm -f *.o perror.msg getopt_lib.msg regcomp.msg

clobber:
	@echo "Clobbering is done by the top-level libc makefile"

#-------------------------------
# Files requiring explicit rules for compilation:
#-------------------------------
#-------------------------------
# Rules for compiling/assembling both profiled and non-profiled .o's,
# on both the s300 and s800
#-------------------------------
.c_p.o:
	$(CC) $(CFLAGS_80) $(PRFLAGS) -c $< -o $@

.c_sl.o:
	$(CC) $(CFLAGS_80) -c $< -o $@

.c_pic.o:
	$(CC) $(CFLAGS_80) -c $< -o $@

.s.o:
	if hp9000s300; then                             	\
		$(M4) $(M4FLAGS) $< | $(AS) $(ASFLAGS) -o $@;   \
	elif hp9000s800; then                                   \
		$(CC) -E $(CPPFLAGS) $< | sed $(SEDFLAGS) 	\
		| cat $(PREFIX) - | $(AS) $(ASFLAGS) -o $@; 	\
	fi;

.s_sl.o:
	if hp9000s300; then                             	\
		$(M4) $(M4FLAGS) $< | $(AS) $(ASFLAGS) -o $@;   \
	elif hp9000s800; then                                   \
		$(CC) -E $(CPPFLAGS) $< | sed $(SEDFLAGS) 	\
		| cat $(PREFIX) - | $(AS) $(ASFLAGS) -o $@; 	\
	fi;

.s_pic.o:
	if hp9000s300; then                             	\
		$(M4) $(M4FLAGS) $< | $(AS) $(ASFLAGS) -o $@;   \
	elif hp9000s800; then                                   \
		$(CC) -E $(CPPFLAGS) $< | sed $(SEDFLAGS) 	\
		| cat $(PREFIX) - | $(AS) $(ASFLAGS) -o $@; 	\
	fi;

.s_p.o:
	if hp9000s300; then                             	\
		$(M4) $(M4FLAGS) $< | $(AS) $(ASFLG_P) -o $@;   \
	elif hp9000s800; then                                   \
		$(CC) -E $(CPPFLG_P) $< | sed $(SEDFLAGS) \
		| cat $(PREFIX) - | $(AS) $(ASFLAGS) -o $@; 	\
	fi;

.SUFFIXES:
.SUFFIXES: .o .s .c _p.o _sl.o _pic.o
