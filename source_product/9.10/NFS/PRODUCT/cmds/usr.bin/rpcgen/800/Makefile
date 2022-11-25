#
# @(#)Makefile	$Revision: 1.4.109.1 $	$Date: 91/11/19 14:08:59 $	
#
#
# Makefile for rpc protocol compiler
# Copyright (C) 1987, Sun Microsystems, Inc.
#
SHELL	= /bin/sh

OCFLAGS	= -O ${CFLAGS}
OLFLAGS = ${LDFLAGS}
TCFLAGS	= -DTRACEON ${CFLAGS}
TLFLAGS	= 
DCFLAGS	= -g -DDEBUG -DTRACEON ${CFLAGS}
DLFLAGS	= -g 
BCFLAGS	= -DBFA ${CFLAGS}
BLFLAGS	= ${LDFLAGS}

CSOURCE = ../rpc_hout.c ../rpc_cout.c ../rpc_parse.c ../rpc_scan.c \
	  ../rpc_util.c ../rpc_svcout.c ../rpc_cltout.c ../rpc_main.c 
800OBJS = rpc_hout.o rpc_cout.o rpc_parse.o rpc_scan.o rpc_util.o \
	  rpc_svcout.o rpc_cltout.o rpc_main.o 
BFAOBJS = rpc_main.b rpc_hout.b rpc_cout.b rpc_parse.b rpc_scan.b \
	  rpc_util.b rpc_svcout.b rpc_cltout.b
HDRS = rpc_util.h rpc_parse.h rpc_scan.h


800TARG = rpcgen
BFATARG = rpcgen.b


all: 		${800TARG}

bfa:		${BFATARG}

rpcgen: ${800OBJS}
	@-rm -f $@
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS}  -o $@ ${800OBJS} ${LIBS} ${LIBC}

rpcgen.b:	${CSOURCE} 
	@-rm -f /users/chm/BFA/rpcgen.B ${BFAOBJS}
	${BFACC} ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ${CSOURCE} -B"-d/users/chm/BFA/rpcgen.B" ${BLIBC} 

rpc_main.o:	../rpc_main.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

rpc_hout.o:	../rpc_hout.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

rpc_cout.o:	../rpc_cout.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

rpc_parse.o:	../rpc_parse.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

rpc_scan.o:	../rpc_scan.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

rpc_util.o:	../rpc_util.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

rpc_svcout.o:	../rpc_svcout.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

rpc_cltout.o:	../rpc_cltout.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f _*.c *.b 

clobber:	clean
	-rm -f ${800TARG} ${BFATARG}

trace:;	$(MAKE) CFLAGS="${TCFLAGS}" LDFLAGS="${TLFLAGS}" all

debug:;	$(MAKE) CFLAGS="${DCFLAGS}" LDFLAGS="${DLFLAGS}" all

prod:;	$(MAKE) CFLAGS="${OCFLAGS}" LDFLAGS="${OLFLAGS}" clean all

linstall:	all
		${INS} rpcgen usr/bin root other 0555 

install:	all
	test "${INS}" && ${INS} ${800TARG}

depend:;
	maketd ${CFLAGS} ${IDIRS} ${CSOURCE} > ,makedep
	@echo '/^# DO NOT DELETE THIS LINE/+1,$$d' > ,eddep
	@echo '$$r ,makedep' >> ,eddep
	@echo 'w' >> ,eddep
	cp Makefile Makefile.bak
	ed - Makefile < ,eddep
	rm -f ,eddep ,makedep
	@echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	@echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	@echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- maketd depends upon it


