# $Source: /misc/source_product/9.10/commands.rcs/lib/libc/net/makefile,v $
# $Revision: 70.1 $

# Common makefile for libc.net subdirectory
# Description file for net: miscellaneous networking routines

OBJS = getrpcent.o gtnetgrent.o innetgr.o exportent.o realpath.o
OBJS_P = $(OBJS:.o=_p.o)
OBJS_SL = $(OBJS:.o=_sl.o)
OBJS_PIC = $(OBJS:.o=_pic.o)

default: archive

archive: $(OBJS) 
	@echo "\tLinking objects to ../tmp..."
	@if [ ! -d ../tmp ]; then		\
	    mkdir ../tmp >/dev/null 2>&1;	\
	fi;					\
	exit 0
	@ln $(OBJS) ../tmp
	touch ../tmp/OK.net

shared: $(OBJS_SL) 
	@echo "\tLinking objects to ../tmp_sl..."
	@if [ ! -d ../tmp_sl ]; then		\
	    mkdir ../tmp_sl >/dev/null 2>&1;	\
	fi;					\
	exit 0
	@ln $(OBJS_SL) ../tmp_sl
	touch ../tmp_sl/OK.net

profile: $(OBJS_P)
	@echo "\tLinking objects to ../tmp_p..."
	@if [ ! -d ../tmp_p ]; then		\
	    mkdir ../tmp_p >/dev/null 2>&1;	\
	fi;					\
	exit 0
	@ln $(OBJS_P) ../tmp_p
	touch ../tmp_p/OK.net

archive_pic: $(OBJS_PIC)
	@echo "\tLinking objects to ../tmp_pic..."
	@if [ ! -d ../tmp_pic ]; then		\
	    mkdir ../tmp_pic >/dev/null 2>&1;	\
	fi;					\
	exit 0
	@ln $(OBJS_PIC) ../tmp_pic
	touch ../tmp_pic/OK.net

clean:
	/bin/rm -f $(OBJS) $(OBJS_P) $(OBJS_SL) $(OBJS_PIC)

clobber:
	@echo "Clobbering is done by the top-level libc makefile"

#
# Define rule so that make knows how to create the different flavors of
# .o files
#
.c_p.o:
	$(CC) $(CFLAGS) $(PRFLAGS) -c $< -o $@

.c_sl.o:
	$(CC) $(CFLAGS) -c $< -o $@

.c_pic.o:
	$(CC) $(CFLAGS) -c $< -o $@

#
# Clear SUFFIXES, then reset defaults so that if a .c and a .s source
# exist for a file, the .s will be compiled before the .c
#
.SUFFIXES:
.SUFFIXES: .o .s .c _p.o _sl.o _pic.o
