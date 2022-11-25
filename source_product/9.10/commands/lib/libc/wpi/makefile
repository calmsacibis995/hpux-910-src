# @(#) $Revision: 70.14 $
#
# Common makefile for libc/wpi subdirectory -- WPI functions
#
#
# The following targets are defined:
#
#    default:	libwpi.a and libwpi.sl
#    libwpi.a:	archive library
#    libwpi.sl:	shared library
#    pfa:       path flow analysis version
#    clean:	delete all .o's
#    clobber:	delete all .o's and libraries
#

OBJECTS= \
	fgetwc.o \
	fgetws.o \
	fputwc.o \
	fputws.o \
	getwc.o \
	getwchar.o \
	putwc.o \
	putwchar.o \
	ungetwc.o \
	wcscat.o \
	wcschr.o \
	wcscmp.o \
	wcscoll.o \
	wcscpy.o \
	wcscspn.o \
	wcsftime.o \
	wcslen.o \
	wcsncat.o \
	wcsncmp.o \
	wcsncpy.o \
	wcspbrk.o \
	wcsrchr.o \
	wcsspn.o \
	wcstod.o \
	wcstok.o \
	wcstol.o \
	wcstoul.o \
	wcswcs.o \
	wcswidth.o \
	wcsxfrm.o \
	wctype.o \
	wctype_fn.o \
	wcwidth.o

OBJECTS_SL  = $(OBJECTS:.o=_sl.o)
OBJECTS_P   = $(OBJECTS:.o=_p.o)
OBJECTS_PIC = $(OBJECTS:.o=_pic.o)

default:	archive

archive:
	@$(MAKE) OBJS="$(OBJECTS)" DIR=tmp wpi

shared:
	@$(MAKE) OBJS="$(OBJECTS_SL)" DIR=tmp_sl wpi

profile:
	@$(MAKE) OBJS="$(OBJECTS_P)" DIR=tmp_p wpi

archive_pic:
	@$(MAKE) OBJS="$(OBJECTS_PIC)" DIR=tmp_pic wpi

wpi: $(OBJS)
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
	$(CC) -Aa -D_XOPEN_SOURCE $(CFLAGS) -c $<

#
# Rules so that make will know how to create object files named foo_p.o,
# foo_sl.o, and foo_pic.o from a .c file named foo.c
#
.c_p.o:
	$(CC) -Aa -D_XOPEN_SOURCE $(CFLAGS) $(PRFLAGS) -c $< -o $@

.c_sl.o:
	$(CC) -Aa -D_XOPEN_SOURCE $(CFLAGS) -c $< -o $@

.c_pic.o:
	$(CC) -Aa -D_XOPEN_SOURCE $(CFLAGS) -c $< -o $@

# The following rules are needed to allow inline assembly for the s300
# version of wcstod.c:
wcstod.o:
	$(CC) -Aa -D_XOPEN_SOURCE $(CFLAGS) +e -c $<
wcstod_p.o:
	$(CC) -Aa -D_XOPEN_SOURCE $(CFLAGS) $(PRFLAGS) +e -c $< -o $@
wcstod_sl.o:
	$(CC) -Aa -D_XOPEN_SOURCE $(CFLAGS) +e -c $< -o $@
wcstod_pic.o:
	$(CC) -Aa -D_XOPEN_SOURCE $(CFLAGS) +e -c $< -o $@

#
# Clear SUFFIXES, then reset the default order of .c and .s suffixes so
# if both exist, the .s will be compiled before the .c
#
.SUFFIXES:
.SUFFIXES: .o .s .c _sl.o _p.o _pic.o
