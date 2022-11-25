# $Header: makefile,v 66.5.1.6 94/08/12 15:23:52 root Exp $
# @(#) $Revision: 66.5.1.6 $
#
# Reboot and Shutdown makefile

FLAGS =

OBJS_REBOOT = reboot.o localDisk.o
LIBS_REBOOT = -lDUX
MSG_REBOOT  = reboot.c localDisk.c

OBJS_SHUTDOWN = shutdown.o localDisk.o
LIBS_SHUTDOWN = -lDUX
MSG_SHUTDOWN  = shutdown.c localDisk.c

FINDMSG	= /usr/bin/findmsg
GENCAT	= /usr/bin/gencat

default: reboot shutdown

reboot: $(OBJS_REBOOT) reboot.cat
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then	\
	    LIBS_REBOOT="$(LIBS_REBOOT) -lscmd -lsec -lcurses";	\
	else							\
	    LIBS_REBOOT="$(LIBS_REBOOT)";			\
	fi;							\
	$(CC) -Wl,-a,archive $(CFLAGS) $(OBJS_REBOOT) $${LIBS_REBOOT} $(LDFLAGS) -o reboot

reboot.cat: reboot.msg
	$(GENCAT) $@ reboot.msg

reboot.msg: $(MSG_REBOOT)
	rm -f reboot.msg
	for i in $(MSG_REBOOT); do       \
	    $(FINDMSG) $$i >>reboot.msg; \
	done

shutdown: $(OBJS_SHUTDOWN) shutdown.cat
	if ifdef SecureWare || ifdef B1 || ifdef TRUSTED; then		\
	    LIBS_SHUTDOWN="$(LIBS_SHUTDOWN) -lscmd -lsec -lcurses";	\
	else								\
	    LIBS_SHUTDOWN="$(LIBS_SHUTDOWN)";				\
	fi;								\
	$(CC) -Wl,-a,archive $(CFLAGS) $(OBJS_SHUTDOWN) $${LIBS_SHUTDOWN} $(LDFLAGS)	\
	-o shutdown

shutdown.cat: shutdown.msg
	$(GENCAT) $@ shutdown.msg

shutdown.msg: $(MSG_SHUTDOWN)
	rm -f shutdown.msg
	for i in $(MSG_SHUTDOWN); do       \
	    $(FINDMSG) $$i >>shutdown.msg; \
	done

clean:
	/bin/rm -f core *.o *.msg *.cat

clobber: clean
	/bin/rm -f reboot shutdown reboot.cat shutdown.cat

rebootLint: reboot
	echo $(CCOPTS) >rb.ccopts
	/usr/bin/awk ' BEGIN { printf ("LINT_OPTS=\""); }          \
		{                                                  \
		  for (i = 1; i < NF; i++)                         \
		    {                                              \
		      start = substr ($$i, 1, 2);                  \
		      if ((start == "-D") || (start == "-U"))      \
			printf ("%s ", $$i);                       \
		    }                                              \
		}                                                  \
	      END { printf ("\"\n");                               \
		    printf ("/usr/bin/lint $$LINT_OPTS $$*\n");    \
                  }                                                \
	    ' rb.ccopts >rb.dolint
	chmod +x rb.dolint
	cp rb.dolint /tmp
	/tmp/rb.dolint $(MSG_REBOOT) > rb.lint.res
	rm -f /tmp/rb.dolint rb.dolint rb.ccopts

shutdownLint: shutdown
	echo $(CCOPTS) >sd.ccopts
	/usr/bin/awk ' BEGIN { printf ("LINT_OPTS=\""); }          \
		{                                                  \
		  for (i = 1; i < NF; i++)                         \
		    {                                              \
		      start = substr ($$i, 1, 2);                  \
		      if ((start == "-D") || (start == "-U"))      \
			printf ("%s ", $$i);                       \
		    }                                              \
		}                                                  \
	      END { printf ("\"\n");                               \
		    printf ("/usr/bin/lint $$LINT_OPTS $$*\n");    \
                  }                                                \
	    ' sd.ccopts >sd.dolint
	chmod +x sd.dolint
	cp sd.dolint /tmp
	/tmp/sd.dolint $(MSG_SHUTDOWN) > sd.lint.res
	rm -f /tmp/sd.dolint sd.dolint sd.ccopts

.c.o:
	$(CC) -c $(CFLAGS) $(FLAGS) $*.c

# copy /etc/reboot to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/reboot
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	reboot
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp reboot ../../../Filesets.sl/UX-CORE/etc
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/reboot
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/reboot to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/reboot
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp reboot ../../../Filesets.sl/UX-CORE/etc/reboot
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/reboot
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/reboot to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/reboot
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp shutdown ../../../Filesets.sl/UX-CORE/etc/shutdown
	! chmog 04555 root bin ../../../Filesets.sl/UX-CORE/etc/shutdown
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
