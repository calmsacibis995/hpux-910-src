CC     = cc -g
CC     = cc
CFLAGS = -DHPUX -DUNIX -DEXEC -DFORK -DBFA_KERNEL -DS300

OBJS = \
	lex.yy.o	\
	cntlscan.o	\
	generate.o	\
	main.o		\
	database.o	\
	file.o		\
	global.o	

EXECS = \
	bfa		\
	bfarpt		\
	bfakupdate	\
	libbfa.a	

all : $(EXECS)
	@echo bfa ready to run

bfa : $(OBJS)
	$(CC) -o bfa $(OBJS) 

lex.yy.o : lex.yy.c global.h defines.h
	$(CC) -c $(CFLAGS) lex.yy.c

lex.yy.c : scanner.l
	lex scanner.l

cntlscan.o : cntlscan.c global.h defines.h
	$(CC) -c $(CFLAGS) cntlscan.c

main.o : main.c global.h defines.h
	$(CC) -c $(CFLAGS) main.c

generate.o : generate.c global.h defines.h
	$(CC) -c $(CFLAGS) generate.c

database.o : database.c defines.h database.h
	$(CC) -c $(CFLAGS) database.c

file.o : file.c defines.h global.h
	$(CC) -c $(CFLAGS) file.c

global.o : global.c defines.h
	$(CC) -c $(CFLAGS) global.c

# Support Routines

bfarpt : bfarpt.o 
	$(CC) -o bfarpt  $(CFLAGS) bfarpt.o

bfarpt.o : bfarpt.c defines.h global.h database.h
	$(CC) -c $(CFLAGS) bfarpt.c

bfakupdate : bfakupdate.o libbfa.o
	$(CC) -o bfakupdate  $(CFLAGS) bfakupdate.o libbfa.o

bfakupdate.o : bfakupdate.c defines.h global.h database.h
	$(CC) -c $(CFLAGS) bfakupdate.c

# BFA Runtime Libarary

libbfa.a : libbfa.o libkbfa.o
	ar r libbfa.a libbfa.o libkbfa.o

libbfa.o : libbfa.c defines.h global.h database.h
	cc -c $(CFLAGS) libbfa.c

libkbfa.o : libkbfa.c defines.h global.h database.h
	cc -c $(CFLAGS) libkbfa.c

clean :
	rm -f *.o lex.yy.c cscope.out

clobber : clean
	rm -f  $(EXECS)

print : clean
	pr -l60 -n *.l *.c *.h  MS-DOS/*.c MS-DOS/*.h MS-DOS/*.asm| laser -elite

tarit : clobber
	tar cf - . > ~/tmp/bfa.tar


tarexecs : $(EXECS)
	tar cfv - $(EXECS) >~/tmp/bfaexec.tar

