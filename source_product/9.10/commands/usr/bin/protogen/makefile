# $Header: makefile,v 72.1.1.5 94/07/29 09:28:19 root Exp $
# @(#) $Revision: 72.1.1.5 $
#
# makefile for /usr/bin/protogen
# add hp9000s700            pkwan 920420
VERSION = hpux_rel
SOURCES = main.c scan.l
OBJECTS = main.o scan.o
LEX = lex

default:	protogen endif

protogen:
	@$(MAKE) TARGET=_protogen setvars

endif:	endif.sh
	cp endif.sh endif

clean:
	@$(MAKE) TARGET=_clean setvars

clobber: clean
	rm -f protogen endif

check:
	@$(MAKE) TARGET=_check setvars

_protogen:	$(OBJECTS) $(VERSION).o
	$(CC) $(FLAGS) $(LDFLAGS) -o protogen $(OBJECTS) $(VERSION).o $(LIBS)

_clean:
	rm -f $(OBJECTS) $(VERSION).o scan.c

_check:
	lint $(SOURCES) $(VERSION).c >protogen.chk

setvars:
	@if hp9000s800; then \
	   flgs=""; \
	   objs="main.o scan.o"; \
	   srcs="main.c scan.l"; \
        elif hp9000s700; then \
	   flgs=""; \
           objs="main.o scan.o"; \
           srcs="main.c scan.l"; \
	elif hp9000s300; then \
	   flgs=""; \
	   objs="main.o scan.o"; \
	   srcs="main.c scan.l"; \
	else \
	   echo "Unknown target system"; \
	fi; \
	if [ -z "$(TARGET)" ]; then \
	   tgt=_protogen; \
	else \
	   tgt="$(TARGET)"; \
	fi; \
	$(MAKE) "FLAGS=$$flgs" "SOURCES=$$srcs" "OBJECTS=$$objs" $$tgt

main.o:	main.c tab.h
scan.c: scan.l
	$(LEX) scan.l
	mv lex.yy.c scan.c
scan.o:	scan.c tab.h
.c.o:
	$(CC) $(FLAGS) -c $<

# copy /usr/bin/protogen to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/protogen
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	protogen
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/bin
	! cp protogen ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin/protogen
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
# copy /usr/bin/protogen to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/protogen
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/bin
	! cp endif ../../../../Filesets.sl/C-TOOLS/usr/bin/endif
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin/endif
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/protogen to the . relative fileset C-TOOLS
# change mode, owner, and group on /usr/bin/protogen
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-TOOLS/usr/bin
	! cp protogen ../../../../Filesets.sl/C-TOOLS/usr/bin/protogen
	! chmog 0555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin/protogen
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-TOOLS/
	! chmog 555 bin bin ../../../../Filesets.sl/
