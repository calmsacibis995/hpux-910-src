SRCS= remsh.c
OBJS= remsh.o
INCLUDES=	-I. -I../include
LIBS=		../util/libutil.a

CFLAGS=		${INCLUDES} -O
LDFLAGS=	-s ${LIBS}

all:  remsh

remsh: ${OBJS} ${LIBC}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LDFLAGS}

clean:
	rm -f ${OBJS} core 

clobber: clean
	rm -f remsh

