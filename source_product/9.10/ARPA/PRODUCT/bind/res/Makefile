/*
 * $Header: Makefile,v 1.25.109.4 93/11/11 15:58:36 randyc Exp $
 */

.SOURCE :   $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/named

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

SHELL		= /bin/sh

ADDOPTS= -DCRYPT -DSHARED_LIBS -DSYMLINKS -DHP_NFS -DNFS -DHPNFS -DNFS3_2 -DDISKLESS -DDUX -DCNODE_DEV -DLONGFILENAMES -DLONGFILENAME -DBSDJOBCTL -DQUOTA -DOLD_RFA -DSWFS -DHDBuucp -DNLS -DNLS8 -DNLS16 -DHFS -DCDROM -DPSTAT -D_NAMESPACE_CLEAN -DUT_HOST -DACLS -DAUDIT -DSHADOWPWD -DGETMOUNT -DLOCAL_DISK -DGT_64_FDS -DEUC

SRC		= gethostent.c \
		  herror.c \
		  res_comp.c res_debug.c res_init.c \
		  res_mkqry.c res_query.c res_send.c _res.c \
		  switch.c

LIBRESOLV	= gethostent.o herror.o res_comp.o res_debug.o res_init.o \
		  res_mkqry.o res_query.o res_send.o _res.o \
		  switch.o

#ifdef BFA
CCFLAGS	= -DBFA -DHP_NFS 
CC=bfacc
#else
CCFLAGS	= -DHP_NFS 
#endif

all: $(TARGET:/^$/resolver/)
	
build: $(TARGET:/^$/resolver/)
	
bfa: $(TARGET:/^$/resolver/)

resolver:: $(SRC)
	ar cru libresolv.a $(LIBRESOLV)

gethostent.o: named/gethostent.c
	$(CC) $(ADDOPTS) -O -c named/gethostent.c

install: libresolv.a

ccount:
	ccount -p -I../include -DSYSV *.c 

clean:
	$(RM) $(RMFLAGS) *.o *.b errs a.out core bfadbase

clobber: clean
	$(RM) $(RMFLAGS) libresolv.a libresolvg.a libresolvb.a

.FORCE:
	@rm -f .FORCE


