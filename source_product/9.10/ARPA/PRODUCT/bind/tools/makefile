#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and that due credit is given
# to the University of California at Berkeley. The name of the University
# may not be used to endorse or promote products derived from this
# software without specific prior written permission. This software
# is provided ``as is'' without express or implied warranty.
#
#	@(#)Makefile	4.7 (Berkeley) 4/6/88
#
CFLAGS=	-O
RES= ../res/libresolv.a
LIBC=	/lib/libc.a
SRCS=	nsquery.c nstest.c
OBJS=	nsquery.o nstest.o
INCLUDE=-I../../include

all: nsquery nstest nslookup

nsquery: ${LIBC}
	${CC} -o $@ nsquery.c ${RES}

# test programs and utilities
nstest: ${LIBC}
	@echo "The resolver routines in libc must be compiled with the -DDEBUG"
	@echo "option or nstest will not have any output."
	${CC} ${INCLUDE} -o $@ nstest.c ${RES}

nslookup: FRC
	cd nslookup; make ${MFLAGS} RES=${RES}

clean: FRC
	cd nslookup; make ${MFLAGS} clean
	rm -f ${OBJS} core nsquery nstest

FRC:

