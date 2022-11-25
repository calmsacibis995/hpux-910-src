#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	6.4 (Berkeley) 6/6/87
#

OBJS=	gethostent.o sethostent.o

SRCS=	gethostent.c sethostent.c

CFLAGS= -O ${DEFS} 
CCOPTS= -DCRYPT -DSHARED_LIBS -DSYMLINKS -DHP_NFS -DNFS -DHPNFS -DNFS3_2 -DDISKLESS -DDUX -DCNODE_DEV -DLONGFILENAMES -DLONGFILENAME -DBSDJOBCTL -DQUOTA -DOLD_RFA -DSWFS -DHDBuucp -DNLS -DNLS8 -DNLS16 -DHFS -DCDROM -DPSTAT -D_NAMESPACE_CLEAN -DUT_HOST -DACLS -DAUDIT -DSHADOWPWD -DGETMOUNT -DLOCAL_DISK -DGT_64_FDS -DEUC
INCLUDE=../../include
TAGSFILE= tags
DESTDIR=

.c.o:
	${CC} -c -I${INCLUDE} ${CFLAGS} $*.c

hostlib hostlib_p: ${OBJS} 
	@echo "building normal hostlib"
	@ar cru hostlib ${OBJS}

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f *.o errs a.out core hostlib hostlib_p profiled/*.o tags

