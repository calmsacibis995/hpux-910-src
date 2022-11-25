# $Header: makefile,v 66.11.1.6 94/07/27 12:44:35 root Exp $
# @(#) $Revision: 66.11.1.6 $
#
# makefile -- makefile for /lib/cpp

VERSION = hpux_rel
#CFLAGS=-O  This hoses the UCS build
CC=cc
LIBS=
COMMONFLAGS=-DNLS -DCXREF -DSTAT_ANAL -DPROTOGEN -DC_PLUS_PLUS

default: cpp cpp.cat

cpp:
	@$(MAKE) TARGET=_cpp setvars

debug.cpp:
	@$(MAKE) DFLAG=-g TARGET=_cpp setvars

clean:
	@$(MAKE) TARGET=_clean setvars

clobber: clean
	rm -f cpp

check:
	@$(MAKE) TARGET=_check setvars

_cpp:  $(OBJECTS) $(VERSION).o
	$(CC) $(FLAGS) $(LDFLAGS) $(CFLAGS) -o cpp $(OBJECTS) $(VERSION).o $(LIBS)

_check:
	lint $(SOURCES) $(VERSION).c >cpp.chk

_clean:
	rm -f $(OBJECTS) $(VERSION).o cpy.c cpp.msg cpp.cat

cpy.o:  cpy.c yylex.c

cpy.c:  cpy.y
	yacc cpy.y
	mv y.tab.c cpy.c

cpp.cat: cpp.c yylex.c cpy.c
	findmsg cpp.c yylex.c cpy.c >cpp.msg
	gencat cpp.cat cpp.msg

#
# setvars -- sets FLAGS, OBJECTS and SOURCES to the correct values
#            for the target system and then calls make on $(TARGET)
#
setvars:
	@if hp9000s800; then                               \
	    flgs="$(COMMONFLAGS) -D__hp9000s800 -DENVINCL";      \
	    objs="cpp.o cpy.o set_env.o";                  \
	    srcs="cpp.c cpy.c set_env.c";                  \
	elif hp9000s300; then                              \
	    flgs="$(COMMONFLAGS) -D__hp9000s300";  \
	    objs="cpp.o cpy.o";                            \
	    srcs="cpp.c cpy.c";                            \
	else                                               \
	    echo "Unknown target system";                  \
	    exit 1;                                        \
	fi;                                                \
	if [ -n "$(DFLAG)" ]; then                         \
	    flgs="$$flgs $(DFLAG)";                        \
	fi;                                                \
	if [ -z "$(TARGET)" ]; then                        \
	    tgt=_cpp;                                      \
	else                                               \
	    tgt="$(TARGET)";                               \
	fi;                                                \
	$(MAKE) "FLAGS=$$flgs" "SOURCES=$$srcs" "OBJECTS=$$objs" $$tgt

.c.o:
	$(CC) -c $(CFLAGS) $(FLAGS) $*.c


# copy /lib/cpp to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/cpp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	cpp
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	! cp cpp ../../../Filesets.sl/BUILDENV/lib
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/lib/cpp
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
# copy /lib/cpp to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/cpp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	! cp cpp ../../../Filesets.sl/BUILDENV/lib/cpp
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/lib/cpp
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/cpp to the . relative fileset C-MIN
# change mode, owner, and group on /lib/cpp
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C-MIN/lib
	! cp cpp ../../../Filesets.sl/C-MIN/lib/cpp
	! chmog 0555 bin bin ../../../Filesets.sl/C-MIN/lib/cpp
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/lib
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /lib/cpp to the . relative fileset C-MIN
# change mode, owner, and group on /lib/cpp
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/C-MIN/usr/lib/nls/C
	! cp cpp.cat ../../../Filesets.sl/C-MIN/usr/lib/nls/C/cpp.cat
	! chmog 0444 bin bin ../../../Filesets.sl/C-MIN/usr/lib/nls/C/cpp.cat
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/C-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/
