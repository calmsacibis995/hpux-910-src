# @(#) $Revision: 66.5 $

# Common makefile for libc/bsd subdirectory

COMOBJS = \
	bcmp.o		\
	bcopy.o		\
	bzero.o		\
	ffs.o		\
	index.o		\
	rindex.o	\
	strcasecmp.o	\
	strncasecmp.o	\
	getusershell.o	\
	mkstemp.o

OBJS300 = 

COMOBJS_P = $(COMOBJS:.o=_p.o)
OBJS300_P = $(OBJS300:.o=_p.o)

COMOBJS_SL = $(COMOBJS:.o=_sl.o)
OBJS300_SL = $(OBJS300:.o=_sl.o)

COMOBJS_PIC = $(COMOBJS:.o=_pic.o)
OBJS300_PIC = $(OBJS300:.o=_pic.o)

default:	archive

archive:
	if hp9000s300; then			\
		objs="$(COMOBJS) $(OBJS300)";	\
	elif hp9000s800; then			\
		objs="$(COMOBJS)";		\
	fi;					\
	$(MAKE) OBJS="$$objs" DIR=tmp bsd

shared:
	if hp9000s300; then			\
		objs="$(COMOBJS_SL) $(OBJS300_SL)";	\
	elif hp9000s800; then			\
		objs="$(COMOBJS_SL)";		\
	fi;					\
	$(MAKE) OBJS="$$objs" DIR=tmp_sl bsd

profile:
	if hp9000s300; then				\
		objs="$(COMOBJS_P) $(OBJS300_P)";	\
	elif hp9000s800; then				\
		objs="$(COMOBJS_P)";			\
	fi;						\
	$(MAKE) OBJS="$$objs" DIR=tmp_p bsd

archive_pic:
	if hp9000s300; then				\
		objs="$(COMOBJS_PIC) $(OBJS300_PIC)";	\
	elif hp9000s800; then				\
		objs="$(COMOBJS_PIC)";			\
	fi;						\
	$(MAKE) OBJS="$$objs" DIR=tmp_pic bsd

bsd: $(OBJS)
	@echo "\tLinking objects to ../$(DIR)..."
	@if [ ! -d ../$(DIR) ]; then		\
	    mkdir ../$(DIR) >/dev/null 2>&1;	\
	fi;					\
	exit 0
	@ln $(OBJS) ../$(DIR)
	touch ../$(DIR)/OK.$@

clean:
	/bin/rm -f $(COMOBJS) $(OBJS300) $(COMOBJS_P) $(OBJS300_P)
	/bin/rm -f $(COMOBJS_SL) $(OBJS300_SL) $(COMOBJS_PIC) $(OBJS300_PIC)

clobber:
	@echo "Clobbering is done by the top-level libc makefile"

#
# Define rule so make knows how to create different flavors of .o's
#
.c_p.o:
	$(CC) $(CFLAGS) $(PRFLAGS) -c $< -o $@

.c_sl.o:
	$(CC) $(CFLAGS) -c $< -o $@

.c_pic.o:
	$(CC) $(CFLAGS) -c $< -o $@

# 
# Clear SUFFIXES, then reset ordering so if a .c and .s source exist
# for a file, the .s will be compiled first
#
.SUFFIXES:
.SUFFIXES: .o .s .c _p.o _sl.o _pic.o
