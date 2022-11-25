##
#	@(#)$Revision: 1.29.109.3 $	$Date: 92/07/31 11:30:57 $  
# (#)Makefile 1.1 86/02/05 SMI; from UCB 4.27 83/07/06
# (#)Makefile	2.4 86/04/17 NFSSRC
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

CSOURCE	= ../nfsstat.c ../rpc.rstatd.c ../rpc.rwalld.c ../rpc.ruserd.c \
	  ../rpc.sprayd.c ../rwall.c ../showmount.c ../rpc.mountd.c \
	  ../rpc.ypassd.c ../spray.c ../rpcinfo.c ../rpc.rquota.c \
	  ../exportfs.c
800TARG	= nfsstat rpc.rstatd rpc.rwalld rpc.rusersd rpc.sprayd \
 	  rwall showmount rpc.mountd rpc.yppasswdd spray rpcinfo \
	  rpc.rquotad  exportfs
BFATARG	= nfsstat.b rpc.rstatd.b rpc.rwalld.b rpc.rusersd.b rpc.sprayd.b \
 	  rwall.b showmount.b rpc.mountd.b rpc.ypassd.b spray.b rpcinfo.b\
	  rpc.rquotad.b exportfs.b


all:		${800TARG}

bfa:		${BFATARG}

rpcinfo:	../rpcinfo.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${RPCSVC_LIB} ${LIBS} ${ST_LIBC}

nfsstat:	../nfsstat.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${RPCSVC_LIB} ${LIBS} ${ST_LIBC}

rpc.mountd:	../rpc.mountd.c ../check_exit.c logging.o
	$(CC2) ${CFLAGS} ${IDIRS} -o $@ ../rpc.mountd.c ../check_exit.c logging.o ${RPCSVC_LIB} ${LIBS} ${ST_LIBC}

rpc.rstatd:	../rpc.rstatd.c ../check_exit.c logging.o
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ ../rpc.rstatd.c ../check_exit.c logging.o ${RPCSVC_LIB} ${LIBS} ${ST_LIBC}

rpc.rwalld:	../rpc.rwalld.c ../check_exit.c logging.o
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ ../rpc.rwalld.c ../check_exit.c logging.o ${RPCSVC_LIB} ${LIBS} ${ST_LIBC}

rpc.rusersd:	../rpc.ruserd.c ../check_exit.c logging.o
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ ../rpc.ruserd.c ../check_exit.c logging.o ${RPCSVC_LIB} ${LIBS} ${ST_LIBC}

rpc.sprayd:	../rpc.sprayd.c ../check_exit.c logging.o
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ ../rpc.sprayd.c ../check_exit.c logging.o ${RPCSVC_LIB} ${LIBS} ${ST_LIBC}

rpc.yppasswdd:	../rpc.ypassd.c logging.o
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ ../rpc.ypassd.c logging.o ${RPCSVC_LIB} ${LIBS} ${ST_LIBC} 

rpc.rquotad:	../rpc.rquota.c logging.o
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ ../rpc.rquota.c logging.o ${RPCSVC_LIB} ${LIBS} ${ST_LIBC} 

rwall:	../rwall.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${RPCSVC_LIB} ${LIBS} ${ST_LIBC}

showmount:	../showmount.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${RPCSVC_LIB} ${LIBS} ${ST_LIBC}

spray:	../spray.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ $? ${RPCSVC_LIB} ${LIBS} ${ST_LIBC}

exportfs: ../exportfs.c issubdir.o
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $@ ../exportfs.c issubdir.o ${LIBS} ${LIBC}

issubdir.o: ../issubdir.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

logging.o:	../logging.c
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -c $?

rpcinfo.b:	../rpcinfo.c 
	rm -f /users/chm/BFA/rpcinfo.B $@ _rpcinfo.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBRPC} ${BLIBC} -B"-d/users/chm/BFA/rpcinfo.B" 

nfsstat.b:	../nfsstat.c
	rm -f /users/chm/BFA/nfsstat.B $@ _nfsstat.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBRPC} ${BLIBC} -B"-d/users/chm/BFA/nfsstat.B" 

rpc.mountd.b:	../rpc.mountd.c ../check_exit.c logging.b
	rm -f /users/chm/BFA/rpc.mountd.B $@ _rpc.mountd.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ../rpc.mountd.c ../check_exit.c logging.b ${BLIBRPC} ${BLIBC} -B"-d/users/chm/BFA/rpc.mountd.B" 

rpc.rstatd.b:	../rpc.rstatd.c ../check_exit.c logging.b
	rm -f /users/chm/BFA/rpc.rstatd.B $@ _rpc.rstatd.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ../rpc.rstatd.c ../check_exit.c logging.b ${BLIBRPC} ${BLIBC} -B"-d/users/chm/BFA/rpc.rstatd.B" 

rpc.rwalld.b:	../rpc.rwalld.c ../check_exit.c logging.b
	rm -f /users/chm/BFA/rpc.rwalld.B $@ _rpc.rwalld.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ../rpc.rwalld.c ../check_exit.c logging.b ${BLIBRPC} ${BLIBC} -B"-d/users/chm/BFA/rpc.rwalld.B" 

rpc.rusersd.b:	../rpc.ruserd.c ../check_exit.c logging.b
	rm -f /users/chm/BFA/rpc.rusersd.B $@ _rpc.rusersd.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ../rpc.ruserd.c ../check_exit.c logging.b ${BLIBRPC} ${BLIBC} -B"-d/users/chm/BFA/rpc.rusersd.B" 

rpc.sprayd.b:	../rpc.sprayd.c ../check_exit.c logging.b
	rm -f /users/chm/BFA/rpc.sprayd.B $@ _rpc.sprayd.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ../rpc.sprayd.c ../check_exit.c logging.b ${BLIBRPC} ${BLIBC} -B"-d/users/chm/BFA/rpc.sprayd.B" 

rpc.ypassd.b:	../rpc.ypassd.c logging.b
	rm -f /users/chm/BFA/rpc.ypassd.B $@ _rpc.ypassd.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ ../rpc.ypassd.c logging.b ${BLIBRPC} ${BLIBC} -B"-d/users/chm/BFA/rpc.ypassd.B"  

rwall.b:	../rwall.c
	rm -f /users/chm/BFA/rwall.B $@ _rwall.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBRPC} ${BLIBC} -B"-d/users/chm/BFA/rwall.B" 

showmount.b:	../showmount.c
	rm -f /users/chm/BFA/showmount.B $@ _showmount.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBRPC} ${BLIBC} -B"-d/users/chm/BFA/showmount.B" 

spray.b:	../spray.c
	rm -f /users/chm/BFA/spray.B $@ _spray.c
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBRPC} ${BLIBC} -B"-d/users/chm/BFA/spray.B" 

logging.b:	../logging.c
	rm -f /users/chm/BFA/logging.B $@ _logging.c 
	$(BFACC) ${BCFLAGS} ${IDIRS} -c $? -B"-d/users/chm/BFA/logging.B"
	mv _logging.o logging.b 

lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm -f *.b _*.c 

clobber:	clean
	-rm -f ${800TARG} ${BFATARG}

install:	all
	test "${INS}" && ${INS} ${800TARG}

trace:;	$(MAKE) CFLAGS="${TCFLAGS}" LDFLAGS="${TLFLAGS}" all

debug:;	$(MAKE) CFLAGS="${DCFLAGS}" LDFLAGS="${DLFLAGS}" all

prod:;	$(MAKE) CFLAGS="${OCFLAGS}" LDFLAGS="${OLFLAGS}" clean all

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
