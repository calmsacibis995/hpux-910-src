# @(#) $Revision: 66.3.1.1 $
#
# Common makefile for libBUILD.a
#
# libBUILD.a  -- is a library containing utilities that are not shipped
#                to customers, but are used to build commands.
#

OBJECTS = parsemode.o toolong.o walkfs.o nl_affirm.o
LIB	= libBUILD.a

default: $(LIB) copy_headers

$(LIB):  hpux_rel.o $(OBJECTS)
	/bin/rm -f $(LIB)
	ar qc $(LIB) hpux_rel.o `lorder $(OBJECTS) | tsort`

copy_headers:
	if cmp -s walkfs.h /usr/include/walkfs.h 2>&1; then	\
	    : walkfs.h is up to date;				\
	else							\
	    cp walkfs.h /usr/include/walkfs.h;			\
	    chmod 444   /usr/include/walkfs.h;			\
	    chgrp bin   /usr/include/walkfs.h;			\
	    chown bin   /usr/include/walkfs.h;			\
	fi
clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f $(LIB)

install:
	cp libBUILD.a /lib
