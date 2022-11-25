#
# $Source: /misc/source_product/9.10/commands.rcs/etc/meas_sys/makefile,v $
# $Revision: 66.1.1.2 $
# $Header: makefile,v 66.1.1.2 94/08/02 11:18:20 root Exp $
# $Release$
#
# makefile for the measurement system commands distributed in UXCORE
# starting in 7.0. Pre 7.0, only iscan was in the release, in LANLINK fileset.
# This makefile is constructed for common build process.
#
#	   This is how the commands should be set up on the system:
# msmod		r-xr-xr-x	owner: bin	group: bin
# iscan		r-xr-sr-x	owner: bin	group: sys		
# translate	r-xr-xr-x	owner: bin	group: bin
#


SHELL= /bin/sh

TARGETS= iscan translate msmod 

OFILES = ./iscan.o ./translate.o ./msmod.o ./nap.o ./note.o ./common.o


default: ${TARGETS}

iscan: iscan.o nap.o note.o common.o 
	${CC} ${CFLAGS} iscan.o nap.o note.o common.o -o iscan

translate: translate.o note.o common.o
	${CC} ${CFLAGS} translate.o note.o common.o -o translate

msmod: msmod.o note.o common.o 
	${CC} ${CFLAGS} msmod.o note.o common.o -o msmod


# dependencies of targets and constituent programs
iscan.o: iscan.c note.h
	${CC} ${CFLAGS} -c iscan.c 

common.o: common.c note.h 
	${CC} ${CFLAGS} -DSPARSE_PDIR -c common.c
	#${CC} ${CFLAGS} -c common.c 

msmod.o: msmod.c note.h
	${CC} ${CFLAGS} -c msmod.c 

# Note: built in .c.o rule handles translate.o, note.o and common.o


clean: 
	rm -f ${OFILES}

clobber: clean
	rm -f ${TARGETS} 

install:
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp iscan ../../../Filesets.sl/SYS-ADMIN/etc/iscan
	! cp msmod ../../../Filesets.sl/SYS-ADMIN/etc/msmod
	! cp translate ../../../Filesets.sl/SYS-ADMIN/etc/translate
	! chmog 2555 bin sys ../../../Filesets.sl/SYS-ADMIN/etc/iscan
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/msmod
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/translate
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN
	! chmog 555 bin bin ../../../Filesets.sl
