# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/bc/makefile,v $
# $Revision: 64.2.1.5 $

# Common makefile for bc

SOURCE = bc.y lib.b.sh
FILES = bc.c hpux_rel.c

default: all

all : bc lib.b
	
bc: $(FILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o bc $(FILES)

# 	The y.tab.c file created by yacc is copied to bc.c to indicate the
#	source of the .c file (it came from the yacc source) and so that
#	the internal rules of make can compile the .c

$(FILES):
	$(YACC) bc.y && mv y.tab.c bc.x
	cp bc.x bc.c

lib.b:
	cp lib.b.sh lib.b

clean:
	/bin/rm -f a.out core *.s *.o
	/bin/rm -f bc.c bc.x lib.b

clobber: clean
	/bin/rm -f bc bc.c lib.b bc.x 

# copy /usr/bin/bc to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/bc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	bc
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp bc ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/bc
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/bc to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/bc
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/etc/newconfig
	! cp lib.b ../../../../Filesets.sl/CMDS-AUX/etc/newconfig/lib.b
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-AUX/etc/newconfig/lib.b
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/etc/newconfig
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/etc/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/bc to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/bc
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp bc ../../../../Filesets.sl/CMDS-AUX/usr/bin/bc
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/bc
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
