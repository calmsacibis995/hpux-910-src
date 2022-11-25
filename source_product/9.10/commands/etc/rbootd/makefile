# $Source: /misc/source_product/9.10/commands.rcs/etc/rbootd/makefile,v $
# $Revision: 66.3.1.5 $

# Common makefile for rbootd

# FLAGS   = -DDEBUG
FLAGS   = -DDTC

OBJECTS = hpux_rel.o rbootd.o setup.o session.o client.o util.o \
	  rmp_proto.o getpacket.o
HEADERS = rbootd.h rmp_proto.h

default: rbootd

rbootd : $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) -lDUX

clean:
	/bin/rm -f *.o 

clobber: clean
	/bin/rm -f rbootd

.c.o:
	$(CC) -c $(CFLAGS) $(FLAGS) $<

# Include file dependencies
$(OBJECTS): $(HEADERS)

# copy /etc/rbootd to the . relative fileset RBOOTD
# change mode, owner, and group on /etc/rbootd
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	rbootd
	mkdir -p ../../../Filesets.sl/RBOOTD/etc
	! cp rbootd ../../../Filesets.sl/RBOOTD/etc
	! chmog 0544 bin bin ../../../Filesets.sl/RBOOTD/etc/rbootd
	! chmog 555 bin bin ../../../Filesets.sl/RBOOTD/etc
	! chmog 555 bin bin ../../../Filesets.sl/RBOOTD/
# copy /etc/rbootd to the . relative fileset RBOOTD
# change mode, owner, and group on /etc/rbootd
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/RBOOTD/etc/newconfig
	! cp boottab ../../../Filesets.sl/RBOOTD/etc/newconfig/boottab
	! chmog 0444 bin bin ../../../Filesets.sl/RBOOTD/etc/newconfig/boottab
	! chmog 555 bin bin ../../../Filesets.sl/RBOOTD/etc/newconfig
	! chmog 555 bin bin ../../../Filesets.sl/RBOOTD/etc/
	! chmog 555 bin bin ../../../Filesets.sl/RBOOTD/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/rbootd to the . relative fileset RBOOTD
# change mode, owner, and group on /etc/rbootd
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/RBOOTD/etc
	! cp rbootd ../../../Filesets.sl/RBOOTD/etc/rbootd
	! chmog 0544 bin bin ../../../Filesets.sl/RBOOTD/etc/rbootd
	! chmog 555 bin bin ../../../Filesets.sl/RBOOTD/etc
	! chmog 555 bin bin ../../../Filesets.sl/RBOOTD/
	! chmog 555 bin bin ../../../Filesets.sl/
