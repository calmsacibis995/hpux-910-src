# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/yacc/makefile,v $
# $Revision: 72.1.1.6 $

# Common makefile for yacc

# These are important options that are included automatically by the 
# build automation automatically.  If you are building locally, uncomment
# the below line before starting.
#CFLAGS = -DNLS -DNLS16 

OFILES = y1.o y2.o y3.o y4.o ytables.o hpux_rel.o
SOURCES = $(OFILES:.o=.c)

FINDMSG = /usr/bin/findmsg
GENCAT  = /usr/bin/gencat
SORT    = /usr/bin/sort 

UNIFDEFFLAGS = -UOSF

default: all

all: yacc yacc.cat yaccpar

yacc:	$(OFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(IFLAG) -o $@ $(OFILES)

$(OFILES): dextern files

clean:
	   rm -f $(OFILES) 

clobber:   clean
	   rm -f yacc yacc.cat

yacc.cat: yacc.cat.in
	$(GENCAT) $@ yacc.cat.in

yaccpar: yaccpar.skel
	rm -f yaccpar
	-unifdef $(UNIFDEFFLAGS) yaccpar.skel > yaccpar

# copy /usr/bin/yacc to the . relative fileset C-AUX
# change mode, owner, and group on /usr/bin/yacc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	yacc
	mkdir -p ../../../../Filesets.sl/C-AUX/usr/bin
	! cp yacc ../../../../Filesets.sl/C-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/C-AUX/usr/bin/yacc
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/
# copy /usr/bin/yacc to the . relative fileset C-AUX
# change mode, owner, and group on /usr/bin/yacc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-AUX/usr/bin
	! cp yacc ../../../../Filesets.sl/C-AUX/usr/bin/yacc
	! chmog 0555 bin bin ../../../../Filesets.sl/C-AUX/usr/bin/yacc
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/yacc to the . relative fileset C-AUX
# change mode, owner, and group on /usr/bin/yacc
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-AUX/usr/lib/nls/C
	! cp yacc.cat ../../../../Filesets.sl/C-AUX/usr/lib/nls/C/yacc.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/nls/C/yacc.cat
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/yacc to the . relative fileset C-AUX
# change mode, owner, and group on /usr/bin/yacc
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/C-AUX/usr/lib
	! cp yaccpar ../../../../Filesets.sl/C-AUX/usr/lib/yaccpar
	! chmog 0444 bin bin ../../../../Filesets.sl/C-AUX/usr/lib/yaccpar
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/C-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
