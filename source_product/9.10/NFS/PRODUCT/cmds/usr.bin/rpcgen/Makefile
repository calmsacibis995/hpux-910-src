#
# @(#)Makefile	$Revision: 1.7.109.1 $	$Date: 91/11/19 14:09:03 $	
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

CSOURCE = rpc_main.c rpc_hout.c rpc_cout.c rpc_parse.c rpc_scan.c rpc_util.c \
	  rpc_svcout.c rpc_cltout.c
OBJECTS = rpc_main.o rpc_hout.o rpc_cout.o rpc_parse.o rpc_scan.o rpc_util.o \
	  rpc_svcout.o rpc_cltout.o
BFA_SRC = _rpc_main.c _rpc_hout.c _rpc_cout.c _rpc_parse.c _rpc_scan.c \
	  _rpc_util.c _rpc_svcout.c _rpc_cltout.c
BFA_OBJ = _rpc_main.o _rpc_hout.o _rpc_cout.o _rpc_parse.o _rpc_scan.o \
	  _rpc_util.o _rpc_svcout.o _rpc_cltout.o
HDRS = rpc_util.h rpc_parse.h rpc_scan.h


TARGETS = rpcgen
BFATARG = rpcgen.b

.SUFFIXES:.b
.c.o:
	@-rm -f $*.o
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $< 
.c:
	@-rm -f $*
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $* $< ${LIBS} ${LIBC}

.c.b:
	@-rm -f /nfs/BFA/$*.B $@ _$< _$*.o
	${BFACC} ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? -B"-d/nfs/BFA/$*.B" ${BLIBC} 

all: 		${TARGETS}

bfa:		${BFATARG}

rpcgen: ${OBJECTS}
	@-rm -f $@
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS}  -o $@ ${OBJECTS} ${LIBS} ${LIBC}

rpcgen.b:	${CSOURCE} 
	@-rm -f /nfs/BFA/rpcgen.B ${BFA_SRC} ${BFA_OBJ}
	${BFACC} ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ${CSOURCE} -B"-d/nfs/BFA/rpcgen.B" ${BLIBC} 

lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f _*.c *.b 

clobber:	clean
	-rm -f ${TARGETS} ${BFATARG}

trace:;	$(MAKE) CFLAGS="${TCFLAGS}" LDFLAGS="${TLFLAGS}" all

debug:;	$(MAKE) CFLAGS="${DCFLAGS}" LDFLAGS="${DLFLAGS}" all

prod:;	$(MAKE) CFLAGS="${OCFLAGS}" LDFLAGS="${OLFLAGS}" clean all

linstall:	all
		${INS} rpcgen usr/bin root other 0555 

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


