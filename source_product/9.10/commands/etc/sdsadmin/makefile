# @(#) $Revision: 70.5.1.7 $

SHELL	      = /bin/sh
SHELL	      = /bin/sh
OBJS	      = sdsadmin.o sds_parse.o hpux_rel.o
HDRS	      = sds.h sds_user.h
SRCS	      = sdsadmin.c sds_parse.c hpux_rel.c
PROGRAM	      = sdsadmin
PROGRAM_M     = sdsadmin.M

default: $(PROGRAM)

all: $(PROGRAM)

metrics: $(PROGRAM_M)

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

$(PROGRAM_M): $(SRCS)
	$(METRICS) $(CFLAGS) $(SRCS) -o $@

clean:
	@rm -f $(OBJS) core *_b.o

clobber:
	@rm -f $(OBJS) $(PROGRAM) $(PROGRAM_M) core tags \
	       *_b.o *.M bba.a bbadump.data

sdsadmin.o: $(HDRS) sdsadmin.c
	$(CC) $(CFLAGS) -c -DSDS_BOOT -DSDS_NEW sdsadmin.c


sds_parse.o: $(HDRS)

sdsadmin_b: sdsadmin_b.o sds_parse_b.o hpux_rel.o
	/usr/hp64000/bin/bbagen -v sdsadmin_b.o sds_parse_b.o
	cc sdsadmin_b.o sds_parse_b.o hpux_rel.o bba.a -o sdsadmin_b
	rm -f bbadump.data

sdsadmin_b.o: sdsadmin.c $(HDRS)
	$(CC) $(CFLAGS) -tp,/usr/hp64000/lib/bbacpp -c \
	    sdsadmin.c -o sdsadmin_b.o

sds_parse_b.o: sds_parse.c $(HDRS)
	$(CC) $(CFLAGS) -tp,/usr/hp64000/lib/bbacpp -c \
	    sds_parse.c -o sds_parse_b.o

# copy /etc/sdsadmin to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/sdsadmin
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	sdsadmin
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp sdsadmin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0544 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/sdsadmin
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/sdsadmin to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/sdsadmin
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp sdsadmin ../../../Filesets.sl/SYS-ADMIN/etc/sdsadmin
	! chmog 0544 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/sdsadmin
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
