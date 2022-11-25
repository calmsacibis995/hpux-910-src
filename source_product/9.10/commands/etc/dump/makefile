# $Source: /misc/source_product/9.10/commands.rcs/etc/dump/makefile,v $
# $Revision: 72.6 $

# Common makefile for dump

#
#	dump.h			header file
#	dumpitime.c		reads /etc/ddate
#	dumpmain.c		driver
#	dumpoptr.c		operator interface
#	dumptape.c		handles the mag tape and opening/closing
#	dumptraverse.c		traverses the file system
#	unctime.c		undo ctime
#	dumprmt.c		functions for remoto access
#

METRICS = :
FLAGS = -DPURDUE_EE -DFASTTCPIP 
LIBS = -lBSD 
LIBSB1 = -lBSD -lscmd -lsec
OBJS = hpux_rel.o dumpitime.o dumpmain.o dumpoptr.o dumptape.o \
       dumptraverse.o unctime.o dumprmt.o
SRCS = hpux_rel.c dumpitime.c dumpmain.c dumpoptr.c dumptape.c \
       dumptraverse.c unctime.c dumprmt.c

default: dump
metrics: dump.M

#the 4 libraries in a row don't work! -o had to come in between !
dump: ${OBJS}
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then		\
		${CC} ${LDFLAGS} ${OBJS} -lm6  -o dump $(LIBSB1); 	\
	else								\
		${CC} ${LDFLAGS} ${OBJS}  -o dump $(LIBS); 		\
	fi

dump.M: ${SRCS}
	${METRICS} ${SRCS} ${FLAGS} -o dump.M

clean:
	/bin/rm -f *.o core 

clobber: clean
	/bin/rm -f dump

${OBJS}: dump.h

dump.h: dumprestore.h


# Redefine the .c to .o rule as to include FLAGS
.c.o:
	$(CC) $(CFLAGS) $(FLAGS) -c $<


# copy /etc/dump to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/dump
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	dump
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp dump ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/dump
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
# copy /etc/dump to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/dump
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp dump ../../../Filesets.sl/SYS-ADMIN/etc/dump
	! chmog 0555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/dump
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/dump to the . relative fileset SYS-ADMIN
# change mode, owner, and group on /etc/dump
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! ln ../../../Filesets.sl/SYS-ADMIN/etc/dump ../../../Filesets.sl/SYS-ADMIN/etc/rdump
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/
	! chmog 555 bin bin ../../../Filesets.sl/
