CFLAGS=	-I../include -O

BSDOBJS=bcmp.o bcopy.o bzero.o getwd.o index.o

80_OBJS= getpty.o utimes.o signal.o account.o logwtmp.o getwd.o

70_OBJS=${BSDOBJS} strcasecmp.o getusershell.o logwtmp.o signal.o \
	getpty.o account.o wtmplog.o

65_OBJS=${BSDOBJS} strcasecmp.o getusershell.o logwtmp.o signal.o \
	getpty.o account.o wtmplog.o syslog.o

31_OBJS=${BSDOBJS} strcasecmp.o getusershell.o logwtmp.o signal.o \
	getpty.o account.o wtmplog.o syslog.o

all : libutil

libutil : ${80_OBJS}
	ar rv libutil.a ${80_OBJS}

7_0: ${70_OBJS}
	ar rv libutil.a ${OBJS}

6_5: $(65_OBJS}
	ar rv libutil.a ${OBJS} syslog.o

3_0: 3_1
	ar rv libutil.a ${OBJS}

3_1: ${31_OBJS}
	ar rv libutil.a ${OBJS}

clean:
	rm -f *.o libutil.a

clobber: clean

bcmp.o:
	ar xv /usr/lib/libBSD.a $@

bzero.o:
	ar xv /usr/lib/libBSD.a $@

bcopy.o:
	ar xv /usr/lib/libBSD.a $@

getwd.o:
	ar xv /usr/lib/libBSD.a $@

index.o:
	ar xv /usr/lib/libBSD.a $@

