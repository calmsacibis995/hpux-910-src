# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/nljust/makefile,v $
# $Revision: 64.2.1.5 $

# Common makefile for building nljust

FLAGS 	 = -DUSG
OBJFILES = global.o main.o line.o dojust.o arab_con.o \
	   arab_fnt.o arab_fun.o arab_shp.o hpux_rel.o
MSGSRC 	 = global.msg main.msg line.msg dojust.msg arab_con.msg \
	   arab_fnt.msg arab_fun.msg arab_shp.msg hpux_rel.msg
FINDMSG	 = /usr/bin/findmsg
GENCAT	 = /usr/bin/gencat

default: nljust nljust.cat

nljust:	$(OBJFILES)
	$(CC) $(LDFLAGS) -o nljust $(OBJFILES)

$(OBJFILES): justify.h extern.h arab_def.h 

clean:
	/bin/rm -f $(OBJECTS) *.msg

clobber: clean
	/bin/rm -f nljust nljust.cat

# Redefine the .c to .o rule so that it will also use FLAGS
.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<

nljust.cat: $(MSGSRC)
	$(GENCAT) $@ $(MSGSRC)

.c.msg:
	$(FINDMSG) $*.c >$*.msg

.msg.cat:
	$(GENCAT) $*.cat $*.msg

.SUFFIXES: .cat .msg

# copy /usr/bin/nljust to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/nljust
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	nljust
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp nljust ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/nljust
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
# copy /usr/bin/nljust to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/nljust
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp nljust ../../../../Filesets.sl/NLS-CORE/usr/bin/nljust
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/nljust
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/nljust to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/nljust
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! cp nljust.cat ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/nljust.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/nljust.cat
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
