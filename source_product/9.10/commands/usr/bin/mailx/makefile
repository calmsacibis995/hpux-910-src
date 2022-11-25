# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/mailx/makefile,v $
# $Revision: 66.5.1.11 $

# Common makefile for the mailx utility.

HDR=hdr
FINDMSG=/usr/bin/findmsg
GENCAT=/usr/bin/gencat

DEFINES= -DUSG -DUSG_TTY
FLAGS= -I$(HDR) $(DEFINES)

XSTR=./xstr

OBJS=   aux.o cmd1.o cmd2.o cmd3.o cmd4.o cmdtab.o collect.o config.o \
	edit.o fio.o getdate.o getname.o hostname.o head.o usg.local.o \
	lock.o lex.o list.o main.o names.o optim.o popen.o quit.o send.o \
	stralloc.o temp.o tty.o vars.o sigretro.o lpaths.o extern.o \
	hpux_rel.o system.o str.o 		# str.o must be last 

HDRS=   $(HDR)/rcv.h \
	$(HDR)/configdefs.h \
	$(HDR)/def.h \
	$(HDR)/glob.h \
	$(HDR)/usg.local.h \
	$(HDR)/local.h \
	$(HDR)/uparm.h \
	$(HDR)/sigretro.h

#
# List sources with NLS message catalogs in MSGSRC
#
MSGSRC= aux.msg cmd1.msg cmd2.msg cmd3.msg cmd4.msg collect.msg edit.msg \
	fio.msg hostname.msg lex.msg list.msg main.msg optim.msg quit.msg \
	send.msg stralloc.msg temp.msg tty.msg vars.msg rmmail.msg head.msg 

#
# Special massaging of C files for sharing of strings
#
.c.o:
	$(CC) -E $(CFLAGS) $(FLAGS) $*.c | $(XSTR) -c -
	$(CC) -c $(CFLAGS) $(FLAGS) x.c
	/bin/mv x.o $*.o
	/bin/rm -f x.c

default: all

all:    mailx rmmail mailx.cat

mailx:  $(XSTR) $(OBJS) $(HDRS)
	/bin/rm -f mailx
	$(CC) $(LDFLAGS) -o mailx $(OBJS)

rmmail:
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then	\
		slibs="-lsec";					\
	else							\
		slibs="";					\
	fi;                                                     \
	$(CC) $(LDFLAGS) -o rmmail rmmail.c $$slibs

mailx.cat:	$(MSGSRC)
	$(GENCAT) $@ $(MSGSRC)

clean:
	/bin/rm -f *.o
	/bin/rm -f *.msg
	/bin/rm -f a.out x.c xs.c core strings getdate.c

clobber:	clean
	/bin/rm -f mailx rmmail $(XSTR) mailx.cat

strings: $(XSTR)

str.o:	strings
	$(XSTR)
	$(CC) -c $(CFLAGS) $(FLAGS) xs.c
	/bin/mv xs.o str.o
	/bin/rm -f xs.c

$(XSTR):        xstr.c
	$(CC) -Wl,-a,archive $(LDFLAGS) -o $(XSTR) xstr.c

cmdtab.o:	cmdtab.c
	$(CC) $(CFLAGS) $(FLAGS) -c cmdtab.c

getdate.o:	getdate.y
	yacc getdate.y
	mv y.tab.c getdate.c
	$(CC) $(CFLAGS) $(FLAGS) -c getdate.c

.c.msg:
	$(FINDMSG) $*.c > $*.msg

.msg.cat:
	$(GENCAT) $*.cat $*.msg

.SUFFIXES: .cat .msg

# copy /usr/bin/mailx to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/mailx
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	mailx
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/bin
	! cp mailx ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin/mailx
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
# copy /usr/bin/mailx to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/mailx
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/etc/newconfig
	! cp mailx.rc ../../../../Filesets.sl/MAILERS/etc/newconfig/mailx.rc
	! chmog 0444 bin bin ../../../../Filesets.sl/MAILERS/etc/newconfig/mailx.rc
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/etc/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/etc/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mailx to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/mailx
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/bin
	! cp mailx ../../../../Filesets.sl/MAILERS/usr/bin/mailx
	! chmog 0555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin/mailx
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mailx to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/mailx
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/lib/mailx
	! cp help/mailx.help ../../../../Filesets.sl/MAILERS/usr/lib/mailx/mailx.help
	! chmog 0444 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/mailx/mailx.help
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/mailx
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mailx to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/mailx
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/lib/mailx
	! cp help/mailx.help.~ ../../../../Filesets.sl/MAILERS/usr/lib/mailx/mailx.help.~
	! chmog 0555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/mailx/mailx.help.~
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/mailx
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mailx to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/mailx
# to 02555, bin, and mail
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/lib/mailx
	! cp rmmail ../../../../Filesets.sl/MAILERS/usr/lib/mailx/rmmail
	! chmog 02555 bin mail ../../../../Filesets.sl/MAILERS/usr/lib/mailx/rmmail
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/mailx
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mailx to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/mailx
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/lib/nls/C
	! cp mailx.cat ../../../../Filesets.sl/MAILERS/usr/lib/nls/C/mailx.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/nls/C/mailx.cat
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mailx to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/mailx
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/lib/nls/C/mailx
	! cp help/mailx.help ../../../../Filesets.sl/MAILERS/usr/lib/nls/C/mailx/mailx.help
	! chmog 0444 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/nls/C/mailx/mailx.help
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/nls/C/mailx
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/nls/C/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mailx to the . relative fileset MAILERS
# change mode, owner, and group on /usr/bin/mailx
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/MAILERS/usr/lib/nls/C/mailx
	! cp help/mailx.help.~ ../../../../Filesets.sl/MAILERS/usr/lib/nls/C/mailx/mailx.help.~
	! chmog 0444 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/nls/C/mailx/mailx.help.~
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/nls/C/mailx
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/nls/C/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/MAILERS/
	! chmog 555 bin bin ../../../../Filesets.sl/
