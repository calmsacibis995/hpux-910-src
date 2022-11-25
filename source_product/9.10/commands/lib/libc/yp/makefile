# $Source: /misc/source_product/9.10/commands.rcs/lib/libc/yp/makefile,v $
# $Revision: 66.4 $

# Common makefile for libc/yp subdirectory

OBJS =			\
	yp_all.o	\
	yp_bind.o	\
	yp_enum.o	\
	yp_master.o	\
	yp_match.o	\
	yp_order.o	\
	yperrstrng.o	\
	ypprot_err.o	\
	ypv1_xdr.o	\
	ypxdr.o

OBJS_P = $(OBJS:.o=_p.o)
OBJS_SL = $(OBJS:.o=_sl.o)
OBJS_PIC = $(OBJS:.o=_pic.o)

default:	archive

archive: $(OBJS)
	@echo "\tLinking objects to ../tmp..."
	@if [ ! -d ../tmp ]; then		\
	    mkdir ../tmp >/dev/null 2>&1;	\
	fi;					\
	exit 0
	@ln $(OBJS) ../tmp
	touch ../tmp/OK.yp

shared: $(OBJS_SL)
	@echo "\tLinking objects to ../tmp_sl..."
	@if [ ! -d ../tmp_sl ]; then		\
	    mkdir ../tmp_sl >/dev/null 2>&1;	\
	fi;					\
	exit 0
	@ln $(OBJS_SL) ../tmp_sl
	touch ../tmp_sl/OK.yp

profile: $(OBJS_P)
	@echo "\tLinking objects to ../tmp_p..."
	@if [ ! -d ../tmp_p ]; then		\
	    mkdir ../tmp_p >/dev/null 2>&1;	\
	fi;					\
	exit 0
	@ln $(OBJS_P) ../tmp_p
	touch ../tmp_p/OK.yp

archive_pic: $(OBJS_PIC)
	@echo "\tLinking objects to ../tmp_pic..."
	@if [ ! -d ../tmp_pic ]; then		\
	    mkdir ../tmp_pic >/dev/null 2>&1;	\
	fi;					\
	exit 0
	@ln $(OBJS_PIC) ../tmp_pic
	touch ../tmp_pic/OK.yp

clean:
	/bin/rm -f $(OBJS) $(OBJS_P) $(OBJS_SL) $(OBJS_PIC)

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
# Clear SUFFIXES, then reset ordering so if a .s and a .c source exist
# for a file, the .s will be compiled first
#
.SUFFIXES:
.SUFFIXES: .o .s .c _p.o _sl.o _pic.o
