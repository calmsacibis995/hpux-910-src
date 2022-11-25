#
#	@(#)Makefile	1.9	07/24/91
#
# Makefile for rpc.pcnfsd
#

BIN= rpc.pcnfsd
TST= clnt.pcnfsd

all: $(BIN)  #$(TST)

SVR_OBJS=  pcnfsd_svc.o pcnfsd_xdr.o pcnfsd_v1.o \
	pcnfsd_v2.o pcnfsd_misc.o pcnfsd_cache.o \
	pcnfsd_print.o 

TST_OBJS= pcnfsd_clnt.o pcnfsd_test.o pcnfsd_xdr.o #fake_malloc.o

SVR_SRCS= $(SVR_OBJS:.o=.c)

TST_SRCS= $(TST_OBJS:.o=.c)

HDRS= pcnfsd.h common.h

.KEEP_STATE:

.INIT: $(HDRS)

LINTFLAGS= -hbax

#
# The next line should be uncommented for SunOS and commented out for SVR4
CFLAGS= -g -DWTMP -Dhpux -DSVR4
#CFLAGS= -g  -DWTMP -Dhpux -DRPC_SVC_FG -DSVR4
#CFLAGS= -DUSER_CACHE -DWTMP -Dhpux -DRPC_SVC_FG
# The next line should be commented for SunOS and uncommented out for SVR4
#CFLAGS= -g -D SVR4 -DSHADOW_SUPPORT
#
# The following line seems to work for SunOS and SVR4 (UHC). However
# I have seen an SVR4 setup which requird that explicitly include
# the -l refs. to the nsl and socket libs.
LIBS= -lrpcsvc 
#LIBS= -lrpcsvc  -lnsl -lsocket

#
# rpcgen is somewhat braindamaged when it comes to including
# extensions to initialization and to the core loop. Therefore
# once a version of the generated code and headers are solid,
# it is necessary to comment out the targets for the generated
# files:
#	pcnfsd_xdr.c
#	pcnfsd_svc.c
#	pcnfsd_clnt.c
#	pcnfsd.h
#
# These are then placed under SCCS control and hacked.
# If you are porting to a new environment and things don't
# seem to work right, you may want to see what your local
# rpcgen will generate. To do this, save the original
# files, uncomment these targets, and make. (Or just run
# rpcgen by hand.) Compare things, especially the set of
# header files fulled in.
#
#pcnfsd_xdr.c: pcnfsd.h pcnfsd.x
#	rpcgen -c pcnfsd.x -o $@ 
#
#pcnfsd_svc.c: pcnfsd.h pcnfsd.x
#	rpcgen -s udp -I pcnfsd.x -o $@
#
#pcnfsd.h: pcnfsd.x
#	rpcgen -h pcnfsd.x -o $@
#pcnfsd_clnt.c: pcnfsd.h pcnfsd.x
#	rpcgen -l pcnfsd.x -o $@


$(BIN): $(SVR_OBJS)
	set -x; cc -o $@  $(SVR_OBJS) $(LIBS) /usr/lib/end.o 
#	cc -o $@  $(SVR_OBJS) /usr/lib/end.o -lrpcsvc -L /usr/local/lib -lcXmem -lcl  

$(TST): $(TST_OBJS)
	cc -o $@ $(TST_OBJS) /usr/lib/end.o $(LIBS)

install: $(BIN)
	install -d $(DESTDIR)/usr/etc
	install -s $(BIN) $(DESTDIR)/usr/etc

tags: $(SRCS)
	ctags $(SVR_SRCS)

lint: $(SRCS)
	lint $(CFLAGS) $(LINTFLAGS) $(SVR_SRCS)
	lint $(CFLAGS) $(LINTFLAGS) $(TST_SRCS)

clean:
	rm -f $(BIN) $(SVR_OBJS) $(TST) $(TST_OBJS)
