#	@(#)$Revision: 1.25.109.2 $	$Date: 92/03/06 17:41:03 $
##
#	Makefile for NFS 800 services
#
SHELL 	= /bin/sh

COMMANDS = usr.lib/libdbm/800 usr.lib/librpcsvc/800 bin/800 etc/800 ucb/800 usr.bin/800 usr.etc/800 usr.etc/yp/800 usr.etc/yp/revnetgroup/800 usr.etc/ypserv/800 usr.etc/vhe/800

RPCSVC_LIB = ${ST_NFS}/cmds/usr.lib/librpcsvc/800/librpcsvc800.a
DBM_LIB = ${ST_NFS}/cmds/usr.lib/libdbm/800/libdbm800.a
CFLAGS= -O -DHP_NFS -DNFS -DNLS

# default (for ISO): search our directory for dbm.h first

IDIRS= -I${ST_NFS}/include

# default (for ISO): build just the services, assuming libc has already been 
#	             built.

PRODUCT	=  ${COMMANDS}

all prod trace debug clean clobber bfa cleanbfa linstall:
	for dir in ${PRODUCT}; do \
	    test -d $${dir} && ( cd $${dir}; $(MAKE) . CFLAGS="${CFLAGS}" IDIRS="${IDIRS}" RPCSVC_LIB="${RPCSVC_LIB}" DBM_LIB="${DBM_LIB}" $@ ) \
	done

install: .FORCE
	test "${INS}" && \
	    for dir in ${PRODUCT}; do \
		test -d $${dir} && ( cd $${dir} ; $(MAKE) . CFLAGS="${CFLAGS}" IDIRS="${IDIRS}" RPCSVC_LIB="${RPCSVC_LIB}" DBM_LIB="${DBM_LIB}" $@ ) ; \
	    done

.FORCE:
	@rm -rf .FORCE

