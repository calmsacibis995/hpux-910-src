# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/finger/makefile,v $
# $Revision: 66.1.1.4 $

# Common makefile for finger

FLAGS	= -DHP

default: finger

finger: finger.o lasttime.o hpux_rel.o netfinger.o
	$(CC) $(CFLAGS) $(FLAGS) $(LDFLAGS) finger.o lasttime.o hpux_rel.o netfinger.o -o $@

clean:
	/bin/rm -f *.o core

clobber: clean
	/bin/rm -f finger

# We re-define the .c.o rule so that implicit compiles (cc -c) will
# use the FLAGS macro as well as CFLAGS.

.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<

# copy /usr/bin/finger to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/finger
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	finger
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp finger ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/finger
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
# copy /usr/bin/finger to the . relative fileset CMDS-AUX
# change mode, owner, and group on /usr/bin/finger
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp finger ../../../../Filesets.sl/CMDS-AUX/usr/bin/finger
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin/finger
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
