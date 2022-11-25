#
#     @(#)Makefile	1.9 90/07/24 4.1NFSSRC SMI
#
# Make file for automount
#
BINS= automount

.SUFFIXES: .x

COPTS = -g
CFLAGS = $(CPPFLAGS) $(COPTS) -Dhpux

SRCS= ../nfs_prot.c ../nfs_server.c ../nfs_trace.c \
	../auto_main.c ../auto_look.c ../auto_proc.c ../auto_node.c \
	../auto_mount.c ../auto_all.c ../putenv.c ../strdup.c ../nfs_cast.c

OBJS= nfs_prot.o nfs_server.o nfs_trace.o \
	auto_main.o auto_look.o auto_proc.o auto_node.o \
	auto_mount.o auto_all.o putenv.o strdup.o nfs_cast.o

HDRS= ../automount.h

LINTFLAGS= -hbax

all: $(BINS)

automount: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ -lrpcsvc

install: $(BINS)
	install -s $(BINS) $(DESTDIR)/usr/etc

nfs_prot.o: ../nfs_prot.c ../nfs_prot.h
	$(CC) $(CFLAGS) -c ../nfs_prot.c

nfs_server.o: ../nfs_server.c ../nfs_prot.h
	$(CC) $(CFLAGS) -c ../nfs_server.c

nfs_trace.o: ../nfs_trace.c ../nfs_prot.h
	$(CC) $(CFLAGS) -c ../nfs_trace.c

auto_main.o: ../auto_main.c ../nfs_prot.h ../automount.h
	$(CC) $(CFLAGS) -c ../auto_main.c

auto_look.o: ../auto_look.c ../nfs_prot.h ../automount.h
	$(CC) $(CFLAGS) -c ../auto_look.c

auto_proc.o: ../auto_proc.c ../nfs_prot.h ../automount.h
	$(CC) $(CFLAGS) -c ../auto_proc.c

auto_node.o: ../auto_node.c ../nfs_prot.h ../automount.h
	$(CC) $(CFLAGS) -c ../auto_node.c

auto_mount.o: ../auto_mount.c ../nfs_prot.h ../automount.h
	$(CC) $(CFLAGS) -c ../auto_mount.c

auto_all.o: ../auto_all.c ../nfs_prot.h ../automount.h
	$(CC) $(CFLAGS) -c ../auto_all.c

putenv.o: ../putenv.c
	$(CC) $(CFLAGS) -c ../putenv.c

strdup.o: ../strdup.c
	$(CC) $(CFLAGS) -c ../strdup.c

nfs_cast.o: ../nfs_cast.c
	$(CC) $(CFLAGS) -c ../nfs_cast.c

tags: $(SRCS)
	ctags $(SRCS)

lint: $(SRCS)
	lint $(LINTFLAGS) $(SRCS)
clean:
	rm -f $(OBJS)

clobber:	clean
	rm -f $(BINS)

depend:
	rm -f makedep
	for i in ${SRCS}; do \
	    ${CC} -M ${INCPATH} $$i | \
	    awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec } ' >> makedep; done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it


putenv.o: ../putenv.c

strdup.o: ../strdup.c
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
