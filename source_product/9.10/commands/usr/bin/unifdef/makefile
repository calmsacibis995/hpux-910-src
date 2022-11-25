# @(#) $Revision: 66.3.1.2 $
#
# Makefile for unifdef (currently unsupported)
#

VERSION = hpux_rel.o
OBJS = unifdef.o y.tab.o yylex.o expr.o

default all:	unifdef

unifdef: $(VERSION) $(OBJS)
	$(CC) $(CFLAGS) $(VERSION) $(OBJS) -o unifdef

y.tab.c y.tab.h: cpy.y
	@/bin/rm -f y.tab.c y.tab.h
	yacc -d cpy.y
	@[ -f y.tab.c ] && chmod a-w y.tab.c
	@[ -f y.tab.h ] && chmod a-w y.tab.h

clean:
	/bin/rm -f *.o core y.tab.c y.tab.h

clobber: clean
	/bin/rm -f unifdef
	
contrib: unifdef
	cp	unifdef	$(ROOT)/usr/contrib/bin/unifdef
	chmod	bin	$(ROOT)/usr/contrib/bin/unifdef
	chgrp	bin	$(ROOT)/usr/contrib/bin/unifdef
	chmod	555	$(ROOT)/usr/contrib/bin/unifdef

oldinstall: unifdef
	$(INS) unifdef

lint:
	lint *.c > lint

unifdef.o: unifdef.h
y.tab.o: unifdef.h y.tab.c
yylex.o: unifdef.h y.tab.h
expr.o:	unifdef.h y.tab.h
y.tab.h: cpy.y
# copy /usr/bin/unifdef to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/unifdef
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
install:
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/bin
	! cp unifdef ../../../../Filesets.sl/C-TOOLS/usr/bin/unifdef
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin/unifdef
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
