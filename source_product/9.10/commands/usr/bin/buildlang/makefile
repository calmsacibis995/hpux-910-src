# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/buildlang/makefile,v $
# $Revision: 70.1.1.5.1.3 $

# Common makefile for buildlang
#
#   ******************** IMPORTNAT NOTE: ****************************
#
# For 9.0 builds, copy the following files to the directory where buildlang
# is being built prior to invoking build:
#
#	/Build/9_0/Build/patchbe/buildlang8/libc.a
#	/Build/9_0/Build/patchbe/buildlang8/langinfo.h
#
# This is due to the fact that 9.0 setlocale is incompatible with the
# 8.* version. Since buildlang uses setlocale (especially in dumplang),
# and due to other possible 8.* to 9.0 changes, it was decided to ship
# an archived version of buildlang for 9.0 which was built with the 
# 8.0 libc.  The reason for using langinfo.h from the 8.0 environment 
# was to get rid of a whole bunch of warnings due to redefinition of 
# of some constants.
#

OBJECTS = table.o era.o main.o global.o nameid.o ctype.o conv.o cswidth.o \
	 coll.o pri.o two1.o one2.o dc.o info.o queue.o error.o hpux_rel.o \
	 dcoll.o debug.o dumplang.o mntry.o modifier.o numeric.o write.o \
	 _start.o

LIBS = -ll


default: libc buildlang instlang

libc:
	sh -x libc.shar

buildlang: $(OBJECTS)
	$(CC) $(CFLAGS) -L. -I. -Wl,-a,archive $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)
# The following was the pre-9.0 rule:
# $(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)


instlang:
	cp instlang.sh instlang

#
#	The following dependencies list the dependent files first, then
#	the file they all depend on (i.e. the .o files all depend on the
#	one .h file).
#

main.o ctype.o table.o: token.h

main.o nameid.o range.o ctype.o conv.o cswidth.o coll.o pri.o two1.o one2.o\
	dc.o chrcol.o info.o era.o queue.o error.o: error.h

main.o global.o nameid.o range.o ctype.o conv.o cswidth.o coll.o pri.o two1.o\
	one2.o dc.o chrcol.o info.o era.o queue.o error.o: global.h lctypes.h

global.o queue.o: define.h

global.o queue.o: types.h

clean:
	/bin/rm -f $(OBJECTS)

clobber: clean
	/bin/rm -f buildlang instlang

#
# The following explicit rules for .c and .l to .o added to pick up
# the local <langinfo.h> include file.  See the "IMPORTNAT" note at the
# beginning of this file.
#
.c.o:
	$(CC) $(CFLAGS) -I. -c $<

.l.o:
	$(LEX) $(LFLAGS) $<
	$(CC) $(CFLAGS) -I. -c lex.yy.c
	rm lex.yy.c
	mv lex.yy.o $@

# copy /usr/bin/buildlang to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/buildlang
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	buildlang
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp buildlang ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/buildlang
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
# copy /usr/bin/buildlang to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/buildlang
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp buildlang ../../../../Filesets.sl/NLS-CORE/usr/bin/buildlang
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/buildlang
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/buildlang to the . relative fileset NLS-CORE
# change mode, owner, and group on /usr/bin/buildlang
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/NLS-CORE/usr/bin
	! cp instlang ../../../../Filesets.sl/NLS-CORE/usr/bin/instlang
	! chmog 0555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin/instlang
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/NLS-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
