# $Source: /misc/source_product/9.10/commands.rcs/bin/sh/makefile,v $
# $Revision: 72.10 $

# The old 300 makefile was also running some of the c files through an
# executable called :fix to make the data segments shared.
# This was removed because of difficulty in implementing on 800 and
# it was not saveing that much space.

# Add -DDUX for DISKLESS/DUX.
# -Uvax and -UNICE were also defined but should not be needed since they
# were being undefined.
#
PFLAGS  = -Wp,-H128000 -DACCT -I../../usr/lib/acct

FINDMSG	= /usr/bin/findmsg
GENCAT	= /usr/bin/gencat

OFILES = \
    setbrk.o blok.o stak.o cmd.o fault.o main.o word.o string.o \
    name.o args.o xec.o service.o error.o io.o print.o macro.o \
    expand.o ctype.o msg.o test.o defs.o echo.o hash.o hashserv.o \
    pwd.o func.o hpux_rel.o
CFILES = \
    setbrk.c blok.c stak.c cmd.c fault.c main.c word.c string.c \
    name.c args.c xec.c service.c error.c io.c print.c macro.c \
    expand.c ctype.c msg.c test.c defs.c echo.c hash.c hashserv.c \
    pwd.c func.c hpux_rel.c

RFA_OFILES = \
    lanlogin.o

RFA_LIB = -ln

MFILES = \
    setbrk.msg blok.msg stak.msg cmd.msg fault.msg main.msg word.msg \
    string.msg name.msg args.msg xec.msg service.msg error.msg io.msg \
    print.msg macro.msg expand.msg ctype.msg msg.msg test.msg defs.msg \
    echo.msg hash.msg hashserv.msg pwd.msg func.msg hpux_rel.msg 

RFA_MFILES = \
    lanlogin.msg 

default: sh sh.cat sh.arch

metrics: sh.M

sh.M: $(CFILES)
	$(METRICS) -o $@ $(CFLAGS) $(PFLAGS) $(CFILES)

sh:
	ofiles="";					\
	lib="";						\
	ifdef RFA && ofiles="$$ofiles $(RFA_OFILES)";	\
	ifdef RFA && lib="$$lib $(RFA_LIB)";		\
	$(MAKE) OBJECTS="$(OFILES) $$ofiles" LIB="$$lib" do_sh

sh.cat:
	mfiles="";					\
	ifdef RFA && mfiles="$$mfiles $(RFA_MFILES)";	\
	$(MAKE) MSGSRC="$(MFILES) $$mfiles" do_sh.cat

do_sh: 	$(OBJECTS)
	$(CC) -Wl,-a,archive $(CFLAGS) $(LDFLAGS) $(OBJECTS) $(LIB) -o sh

sh.arch:      sh
	ofiles="";					\
	lib="";						\
	ifdef RFA && ofiles="$$ofiles $(RFA_OFILES)";	\
	ifdef RFA && lib="$$lib $(RFA_LIB)";		\
	$(MAKE) OBJECTS="$(OFILES) $$ofiles" LIB="$$lib" do_sh.arch

do_sh.arch:	sh
	$(CC) -Wl,-a,archive $(CFLAGS) $(LDFLAGS) $(OBJECTS) $(LIB) -o sh.arch

do_sh.cat: $(MSGSRC)
	rm -f sh.cat
	$(GENCAT) sh.cat $(MSGSRC)

$(OFILES) $(RFA_OFILES): defs.h

# Dependencies
cmd.o main.o msg.o word.o xec.o macro.o: sym.h

defs.o: name.h mode.h

hash.o hashserve.o xec.o: hash.h

io.o main.o: dup.h timeout.h

pwd.o: mac.h

clean:
	  /bin/rm -f *.o *.msg

clobber:  clean
	  /bin/rm -f sh rsh sh.cat sh.arch

# Redefine the .c to .o rule as to include PFLAGS
.c.o:
	$(CC) $(CFLAGS) $(PFLAGS) -c $<

.c.msg: 
	$(FINDMSG) $*.c > $*.msg

.msg.cat:
	$(GENCAT) $*.cat $*.msg

.SUFFIXES: .cat .msg

# copy /bin/sh to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	sh
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp sh ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/sh
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/sh to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp sh ../../../Filesets.sl/UX-CORE/bin/sh
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/sh
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/sh to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sh
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp sh.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/sh.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/sh.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/sh to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sh
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! ln ../../../Filesets.sl/UX-CORE/bin/sh ../../../Filesets.sl/UX-CORE/bin/rsh
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
