# $Source: /misc/source_product/9.10/commands.rcs/etc/rmt/makefile,v $
# $Revision: 72.6 $

# Common makefile for restore

OBJECTS  =  rmt.o hpux_rel.o
SOURCES  =  rmt.c hpux_rel.c
FLAGS= -DFASTTCPIP
INCLUDES = -I../dump
LIBS = -lsec -lscmd

default: rmt

metrics: rmt.M

rmt: $(OBJECTS)
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then          \
		$(CC) $(LDFLAGS) $(OBJECTS) -o rmt $(LIBS);		\
	else								\
		$(CC) $(LDFLAGS) $(OBJECTS) -o rmt;			\
	fi

rmt.M: $(SOURCES)
	$(METRICS) $(FLAGS) $(FLAGS) $(INCLUDES) $(SOURCES) -o rmt.M

rmt.o:	rmt.c
	$(CC) $(CFLAGS) $(FLAGS) $(INCLUDES) -c rmt.c

clean:
	/bin/rm -f *.o core

clobber: clean
	/bin/rm -f rmt

# Redefine the .c to .o rule as to include FLAGS
.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<


# copy /etc/rmt to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/rmt
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	rmt
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp rmt ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/rmt
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/rmt to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/rmt
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp rmt ../../../Filesets.sl/SYS-ADMIN/etc/rmt
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/rmt
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
