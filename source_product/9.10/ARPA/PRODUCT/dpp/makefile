CFLAGS	      = -O 

DEST	      = 

EXTHDRS	      = /usr/include/ctype.h \
		/usr/include/errno.h \
		/usr/include/fcntl.h \
		/usr/include/netdb.h \
		/usr/include/netinet/in.h \
		/usr/include/setjmp.h \
		/usr/include/signal.h \
		/usr/include/stdio.h \
		/usr/include/string.h \
		/usr/include/sys/errno.h \
		/usr/include/sys/getaccess.h \
		/usr/include/sys/ioctl.h \
		/usr/include/sys/signal.h \
		/usr/include/sys/stat.h \
		/usr/include/sys/termio.h \
		/usr/include/sys/time.h \
		/usr/include/sys/types.h \
		/usr/include/time.h \
		/usr/include/unistd.h \
		/usr/include/utime.h \
		/usr/include/utmp.h

HDRS	      = decodex.h \
		display_msg.h \
		display_msgx.h \
		dpp_const.h \
		processx.h \
		store_login.h \
		store_loginx.h

INSTALL	      = install

LD	      = cc 

LDFLAGS	      = 

LIBS	      =

MAKEFILE      = Makefile

OBJS	      = decode.o \
		display_msg.o \
		dpp.o \
		process.o \
		store_login.o

PRINT	      = pr

PROGRAM       = dpp  

SHELL	      = /bin/sh

SRCS	      = decode.c \
		display_msg.c \
		dpp.c \
		process.c \
		store_login.c

all:		$(PROGRAM)

$(PROGRAM):     $(OBJS) $(LIBS)
		@echo "Linking $(PROGRAM) ..."
		@$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $(PROGRAM)
		@echo "done"

clean:;		@rm -f $(OBJS) core

clobber:;	@rm -f $(OBJS) $(PROGRAM) core tags

depend:;	@mkmf -f $(MAKEFILE) ROOT=$(ROOT)

echo:;		@echo $(HDRS) $(SRCS)

index:;		@ctags -wx $(HDRS) $(SRCS)

install:	$(PROGRAM)
		@echo Installing $(PROGRAM) in $(DEST)
		@-strip $(PROGRAM)
		@if [ $(DEST) != . ]; then \
		(rm -f $(DEST)/$(PROGRAM); $(INSTALL) -f $(DEST) $(PROGRAM)); fi

print:;		@$(PRINT) $(HDRS) $(SRCS)

tags:           $(HDRS) $(SRCS); @ctags $(HDRS) $(SRCS)

update:		$(DEST)/$(PROGRAM)

$(DEST)/$(PROGRAM): $(SRCS) $(LIBS) $(HDRS) $(EXTHDRS)
		@$(MAKE) -f $(MAKEFILE) ROOT=$(ROOT) DEST=$(DEST) install
###
decode.o: /usr/include/sys/termio.h /usr/include/sys/ioctl.h \
	/usr/include/sys/getaccess.h /usr/include/sys/types.h \
	/usr/include/netinet/in.h /usr/include/string.h /usr/include/fcntl.h \
	/usr/include/sys/stat.h /usr/include/time.h /usr/include/sys/time.h \
	/usr/include/signal.h /usr/include/sys/signal.h /usr/include/setjmp.h \
	/usr/include/stdio.h /usr/include/ctype.h /usr/include/errno.h \
	/usr/include/sys/errno.h /usr/include/netdb.h /usr/include/unistd.h \
	/usr/include/utime.h /usr/include/utmp.h display_msg.h display_msgx.h \
	dpp_const.h
display_msg.o: /usr/include/sys/termio.h /usr/include/sys/ioctl.h \
	/usr/include/string.h /usr/include/fcntl.h /usr/include/sys/stat.h \
	/usr/include/sys/types.h /usr/include/time.h /usr/include/sys/time.h \
	/usr/include/stdio.h /usr/include/ctype.h /usr/include/errno.h \
	/usr/include/sys/errno.h display_msg.h
dpp.o: /usr/include/sys/termio.h /usr/include/sys/ioctl.h \
	/usr/include/sys/getaccess.h /usr/include/sys/types.h \
	/usr/include/netinet/in.h /usr/include/string.h /usr/include/fcntl.h \
	/usr/include/sys/stat.h /usr/include/time.h /usr/include/sys/time.h \
	/usr/include/signal.h /usr/include/sys/signal.h /usr/include/setjmp.h \
	/usr/include/stdio.h /usr/include/ctype.h /usr/include/errno.h \
	/usr/include/sys/errno.h /usr/include/netdb.h /usr/include/unistd.h \
	/usr/include/utime.h /usr/include/utmp.h display_msgx.h processx.h \
	decodex.h store_loginx.h dpp_const.h display_msg.h
process.o: /usr/include/sys/termio.h /usr/include/sys/ioctl.h \
	/usr/include/sys/getaccess.h /usr/include/sys/types.h \
	/usr/include/netinet/in.h /usr/include/string.h /usr/include/fcntl.h \
	/usr/include/sys/stat.h /usr/include/time.h /usr/include/sys/time.h \
	/usr/include/signal.h /usr/include/sys/signal.h /usr/include/setjmp.h \
	/usr/include/stdio.h /usr/include/ctype.h /usr/include/errno.h \
	/usr/include/sys/errno.h display_msg.h display_msgx.h dpp_const.h
store_login.o: /usr/include/sys/termio.h /usr/include/sys/ioctl.h \
	/usr/include/sys/types.h /usr/include/string.h /usr/include/fcntl.h \
	/usr/include/sys/stat.h /usr/include/stdio.h /usr/include/ctype.h \
	/usr/include/errno.h /usr/include/sys/errno.h /usr/include/unistd.h \
	/usr/include/utime.h store_login.h display_msg.h display_msgx.h \
	dpp_const.h
