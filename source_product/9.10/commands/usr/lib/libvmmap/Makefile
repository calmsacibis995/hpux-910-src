# @(#) $Revision: 70.1 $
#
# makefile for /usr/lib/libvmmap.a
#

INCL_DIR=/usr/include
# CFLAGS=-g -I$(INCL_DIR) -D_HPUX_SOURCE -DDEBUG -DSPARSE_PDIR
# CFLAGS=-O -I$(INCL_DIR) -D_HPUX_SOURCE -DSPARSE_PDIR
# CFLAGS=-G -O -I$(INCL_DIR) -D_HPUX_SOURCE -DSPARSE_PDIR

OBJS= vmmap.o

LIB_OBJS= libvmmap.a(vmmap.o)

SRCS= vmmap.c

default: libvmmap.a

libvmmap.a: $(LIB_OBJS)

# $(OBJS): $(INCL_DIR)/machine/pde.h

lint:
	lint -I$(INCL_DIR) -Dvolatile= -D_HPUX_SOURCE -DSPARSE_PDIR $(SRCS)

libvmmap.a(vmmap.o): vmmap.o
	ar rv libvmmap.a vmmap.o

vmmap.o: vmmap.c
	cc -c $(CFLAGS) vmmap.c

libvmmap.a:	vmmap.o
	rm -f libvmmap.a
	ar -r libvmmap.a vmmap.o

clean:
	rm *.o libvmmap.a

test: libvmmap.a test.o 
	cc -o test -Wl,-a,archive test.o libvmmap.a

test.o: test.c
