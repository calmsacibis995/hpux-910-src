# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/localedef/makefile,v $
# $Revision: 70.5.1.6 $

# Common makefile for localedef

FINDMSG = /usr/bin/findmsg
GENCAT	= /usr/bin/gencat
SORT    = /bin/sort

OBJECTS = table.o copy.o era.o main.o global.o nameid.o ctype.o	\
	  conv.o cswidth.o coll.o symbol.o pri.o  	\
	  info.o queue.o error.o hpux_rel.o grp.o dcoll.o	\
	  debug.o dumplang.o mntry.o modifier.o numeric.o	\
	  symbol.o write.o interpret.o

LIBS = -ll

default: localedef instlang_inf localedef.cat

localedef: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS)  $(OBJECTS)  -o $@  $(LIBS) 

instlang_inf:
	cp instlang_inf.sh instlang_inf

#
#	The following dependencies list the dependent files first, then
#	the file they all depend on (i.e. the .o files all depend on the
#	one .h file).axf
#

main.o ctype.o table.o:	token.h

main.o nameid.o range.o ctype.o conv.o	\
	cswidth.o coll.o symbol.o pri.o	\
	chrcol.o	\
	info.o era.o grp.o queue.o	\
	symbol.o error.o :		error.h

main.o global.o copy.o nameid.o range.o	\
	ctype.o conv.o cswidth.o coll.o	\
	symbol.o pri.o  	\
	chrcol.o info.o era.o 	\
	grp.o symbol.o queue.o error.o:	global.h lctypes.h


global.o queue.o:	define.h

global.o queue.o:	types.h

clean:
	/bin/rm -f $(OBJECTS) *.msg

clobber: clean
	/bin/rm -f localedef instlang_inf localedef.cat

localedef.cat:	localedef.msg
	$(GENCAT) $@ localedef.msg

localedef.msg:
	$(FINDMSG) -a *.c > $@


# copy /usr/bin/localedef to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/localedef
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	localedef
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp localedef ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/localedef
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
# copy /usr/bin/localedef to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/localedef
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp instlang_inf ../../../../Filesets.sl/NLS-CORE/usr/bin/instlang_inf
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/instlang_inf
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/localedef to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/localedef
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp localedef ../../../../Filesets.sl/NLS-CORE/usr/bin/localedef
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/localedef
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/localedef to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/localedef
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! cp localedef.cat ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/localedef.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C/localedef.cat
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
