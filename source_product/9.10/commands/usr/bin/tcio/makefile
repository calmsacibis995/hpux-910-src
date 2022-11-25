# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/tcio/makefile,v $
# $Revision: 72.5 $

# Common makefile for tcio

OBJECTS = hpux_rel.o tcio.o tpsup.o optprs.o tpmdep.o
SOURCES = hpux_rel.c tcio.c tpsup.c optprs.c tpmdep.c
INCLUDES = tcio.h tcio.def

COMMAND = tcio
COMMAND_M = tcio.M

default: $(COMMAND) $(COMMAND).arch

metrics: $(COMMAND_M)

$(COMMAND):  $(OBJECTS) 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(COMMAND) $(OBJECTS)

$(COMMAND_M):  $(SOURCES) 
	$(METRICS) $(CFLAGS) -o $(COMMAND_M) $(SOURCES)

$(COMMAND).arch:  $(OBJECTS) 
	if hp9000s800; then                             		   \
		$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-a,archive -o $@ $(OBJECTS);\
        fi; 

$(OBJECTS):  $(INCLUDES)

clean:
	/bin/rm -f $(OBJECTS)

clobber: clean
	/bin/rm -f tcio tcio.arch


# copy /usr/bin/tcio to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/tcio
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	tcio
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp tcio ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/tcio
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
# copy /usr/bin/tcio to the . relative fileset CMDS-MIN
# change mode, owner, and group on /usr/bin/tcio
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! cp tcio ../../../../Filesets.sl/CMDS-MIN/usr/bin/tcio
	! chmog 0555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin/tcio
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/CMDS-MIN/
	! chmog 555 bin bin ../../../../Filesets.sl/
