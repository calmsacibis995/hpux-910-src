# @(#) $Revision: 70.1.1.1 $
#
# Common makefile for libc/bsdipc subdirectory -- ARPA utilities
#

OBJECTS = \
	_res.o		\
	bindup.o	\
	chk_rhosts.o	\
	gethostent.o	\
	getnetent.o	\
	getservent.o	\
	gprotoent.o	\
	herror.o	\
	inet_addr.o	\
	inet_lnaof.o	\
	inet_maddr.o	\
	inet_netof.o	\
	inet_netwk.o	\
	inet_ntoa.o	\
	rcmd.o		\
	res_comp.o	\
	res_debug.o	\
	res_init.o	\
	res_mkqry.o	\
	res_query.o	\
	res_send.o	\
	ruserok.o	\
	ruserpass.o	\
	switch.o	\
	useyellow.o	\
	yellowup.o

OBJECTS_SL  = $(OBJECTS:.o=_sl.o)
OBJECTS_P   = $(OBJECTS:.o=_p.o)
OBJECTS_PIC = $(OBJECTS:.o=_pic.o)

default:	archive

archive:
	@$(MAKE) OBJS="$(OBJECTS)" DIR=tmp bsdipc

shared:
	@$(MAKE) OBJS="$(OBJECTS_SL)" DIR=tmp_sl bsdipc

profile:
	@$(MAKE) OBJS="$(OBJECTS_P)" DIR=tmp_p bsdipc

archive_pic:
	@$(MAKE) OBJS="$(OBJECTS_PIC)" DIR=tmp_pic bsdipc

bsdipc: $(OBJS)
	@echo "\tLinking objects to ../$(DIR)..."
	@if [ ! -d ../$(DIR) ]; then		\
	    mkdir ../$(DIR) >/dev/null 2>&1;	\
	fi;					\
	exit 0
	@ln $(OBJS) ../$(DIR)
	touch ../$(DIR)/OK.$@

clean:
	@echo "\tRemoving object files..."
	@/bin/rm -f $(OBJECTS) $(OBJECTS_SL) $(OBJECTS_P) $(OBJECTS_PIC)

clobber:
	@echo "clobbering is done by the main libc makefile"

#
# Re-define rule for compiling a .o from a .c
#
.c.o:
	$(CC) $(CFLAGS) -c $<

#
# Rules so that make will know how to create object files named foo_p.o,
# foo_sl.o, and foo_pic.o from a .c file named foo.c
#
.c_p.o:
	$(CC) $(CFLAGS) $(PRFLAGS) -c $< -o $@

.c_sl.o:
	$(CC) $(CFLAGS) -c $< -o $@

.c_pic.o:
	$(CC) $(CFLAGS) -c $< -o $@

#
# Clear SUFFIXES, then reset the default order of .c and .s suffixes so
# if both exist, the .s will be compiled before the .c
#
.SUFFIXES:
.SUFFIXES: .o .s .c _sl.o _p.o _pic.o
