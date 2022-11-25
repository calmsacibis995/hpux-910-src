# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/more/makefile,v $
# $Revisions$

# Common makefile for building more

PFLAGS 	= -DUSG
LIBS  	= -lcurses
FINDMSG	= /usr/bin/findmsg
GENCAT	= /usr/bin/gencat

default:	more more.cat

more:	more.o  
	$(CC) $(LDFLAGS) -o more more.o $(LIBS)

clean:
	/bin/rm -f *.o core more.msg 

clobber: clean
	/bin/rm -f more more.cat

more.cat: more.msg

# Redefine the .c to .o rule so that it will include PFLAGS
.c.o:
	$(CC) $(CFLAGS) $(PFLAGS) -c $<

.c.msg:
	$(FINDMSG) $*.c >$*.msg

.msg.cat:
	$(GENCAT) $*.cat $*.msg

.SUFFIXES: .cat .msg

# copy /usr/bin/more to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/more
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	more
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp more ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/more
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/more to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/more
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp more ../../../../Filesets.sl/CMDS-MIN/usr/bin/more
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/more
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/more to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/more
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/lib
	! cp more.help ../../../../Filesets.sl/CMDS-MIN/usr/lib/more.help
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/more.help
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/more to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/more
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! cp more.cat ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/more.cat
	! chmog 0444 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C/more.cat
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/C
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/nls/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/lib/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/more to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/more
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! ln ../../../../Filesets.sl/CMDS-MIN/usr/bin/more ../../../../Filesets.sl/CMDS-MIN/usr/bin/page
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
