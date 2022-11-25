#
#  $Source: /misc/source_product/9.10/commands.rcs/usr/bin/elm/utils/makefile,v $
#   @(#) $Revision: 66.3 $
# 
#  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
#  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
#
#  Acknowledgment is made to Dave Taylor for his creation of
#  the original version of this software.
# 
# 
#  Makefile for the Elm system utilities
#

SHELL	= /bin/sh

BIN	= ../bin
SRC	= ../src
DEFINE	=
FLAGS	= -I../hdrs

RM	= /bin/rm -f

HEADERS	=			\
	../hdrs/defs.h		\
	../hdrs/sysdefs.h

HEADERS1=			\
	../hdrs/elm.h		\
	../hdrs/headers.h

TARGETS	=		\
	${BIN}/elmalias	\
	${BIN}/mailfrom	\
	${BIN}/newmail	\
	${BIN}/readmail

OBJS	=			\
	${SRC}/getopt.o		\
	${SRC}/opt_utils.o	\
	${SRC}/string2.o	\
	${SRC}/validname.o


default: all
all: ${TARGETS}

${BIN}/elmalias:  ${HEADERS} elmalias.o checkalias.o		\
		listalias.o ../hpux_rel.o ${OBJS}
	${CC} ${CFLAGS} ${FLAGS} ${DEFINE} elmalias.o ${OBJS}	\
		../hpux_rel.o listalias.o checkalias.o -o ${BIN}/elmalias

${BIN}/mailfrom: ${HEADERS} mailfrom.o expand.o ../hpux_rel.o	\
		${SRC}/opt_utils.o ${SRC}/string2.o ${SRC}/getopt.o
	${CC} ${CFLAGS} ${FLAGS} ${DEFINE} mailfrom.o		\
		${SRC}/opt_utils.o ${SRC}/string2.o ../hpux_rel.o	\
		${SRC}/getopt.o expand.o -o ${BIN}/mailfrom

${BIN}/newmail: ${HEADERS} newmail.o expand.o ${SRC}/string2.o ../hpux_rel.o
	${CC} ${CFLAGS} ${FLAGS} ${DEFINE} newmail.o expand.o	\
		../hpux_rel.o ${SRC}/string2.o -o ${BIN}/newmail

${BIN}/readmail: ${HEADERS} readmail.o expand.o ${SRC}/getopt.o	\
		${SRC}/opt_utils.o ${SRC}/string2.o ../hpux_rel.o
	${CC} ${CFLAGS} ${FLAGS} ${DEFINE} readmail.o expand.o	\
		${SRC}/getopt.o ${SRC}/string2.o ../hpux_rel.o	\
		${SRC}/opt_utils.o -o ${BIN}/readmail

${SRC}/validname.o: ${SRC}/validname.c ${HEADERS} ${HEADERS1}
	cd ${SRC};						\
	${CC} -c ${CFLAGS} ${FLAGS} ${DEFINE} validname.o;

${SRC}/opt_utils.o: ${SRC}/opt_utils.c ${HEADERS} ${HEADERS1}
	cd ${SRC};						\
	${CC} -c ${CFLAGS} ${FLAGS} ${DEFINE} opt_utils.o

${SRC}/getopt.o: ${SRC}/getopt.c ${HEADERS} ${HEADERS1}
	cd ${SRC};						\
	${CC} -c ${CFLAGS} ${FLAGS} ${DEFINE} getopt.c

${SRC}/string2.o: ${SRC}/string2.c ${HEADERS} ${HEADERS1}
	cd ${SRC};						\
	${CC} -c ${CFLAGS} ${FLAGS} ${DEFINE} string2.c

../hpux_rel.o: ../hpux_rel.c
	cd ..;							\
	${CC} -c ${CFLAGS} ${FLAGS} ${DEFINE} hpux_rel.c


clean:
	${RM} *.o ${OBJS} ../hpux_rel.o

clobber: clean
	${RM} ${TARGETS}

.c.o: ${HEADERS}
	${CC} -c ${CFLAGS} ${FLAGS} ${DEFINE} $<
