INCLUDE=-I. -I../include
SRCS=	fingerd.c
OBJS=	fingerd.o
LIB=	../util/libutil.a
CFLAGS=	-O -DOPTIONS
LDFLAGS=-s

.c.o:
	${CC} -c ${CFLAGS} ${INCLUDE} $*.c

all: fingerd

fingerd : ${OBJS}
	${CC} -o $@ ${LDFLAGS} ${OBJS} ${LIB}
	
clean: .FORCE
	rm -f *.o *.b core 

clobber: clean
	-rm -f fingerd

.FORCE:
	@rm -f .FORCE
