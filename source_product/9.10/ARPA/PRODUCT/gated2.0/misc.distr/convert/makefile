OPTS= -O
build:	convert

convert: y.tab.o lex.yy.o
	cc $(OPTS) -o conv_config y.tab.o lex.yy.o -ll

y.tab.o: convert.y
	yacc -d convert.y
	cc $(OPTS) -c y.tab.c
	

lex.yy.o: convert.l
	lex convert.l
	cc $(OPTS) -c lex.yy.c
	
clobber:
	rm -f conv_config y.tab.c lex.yy.c *.o y.tab.h
