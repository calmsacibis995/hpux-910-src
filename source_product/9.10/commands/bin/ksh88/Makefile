# HPUX_ID: @(#) $Revision: 66.1 $ 

#
#      Copyright (c) 1984, 1985, 1986, 1987,
#                  1988, 1989  AT&T
#      All Rights Reserved
#
#      THIS IS UNPUBLISHED PROPRIETARY SOURCE 
#      CODE OF AT&T.
#      The copyright notice above does not 
#      evidence any actual or intended
#      publication of such source code.
#
/*
 * ksh makefile
 * This makefile should work with nmake 1.? and nmake 2.? 
 */

.SOURCE : sh shlib install
.SOURCE.h : include 

SH = ksh		/* name of the shell program */

/* documentation files */
DOCFILES = sh.memo sh.1 RELEASEa README RELEASE COMPATIBILITY PROMO

KSHELL = 1		/* this is ksh */

/* set these variables to 1 to enable, null to disable */

ACCT =			/* shell accounting (Sys V only ) */
DBUG =			/* debugging purposes only */
ESH = 1			/* emacs/gmacs edit mode */
ESHPLUS =		/* optional emacs edit mode features */
FLOAT =			/* floating point arithmetic */
JOBS = 1		/* jobs monitor option is selected */
MULTIBYTE =		/* multi-byte character sets */
OLDTEST = 1		/* old test and [ as builtin command */
NEWTEST = 1		/* new [[...]] test compound command */
PDU = 			/* for portible distributed unix */
SEVENBIT = 		/* all characters seven bit */
SUID_EXEC = 1		/* allow (safe) suid/sgid shell scripts */
VSH = 1			/* vi edit mode */
YELLOWP =		/* on for yellow pages */

/* these should be set automatically */

RAWONLY=		/* use raw mode only with vi-mode */
VFORK = 		/* vfork option is selected */

#ifndef __STDPP__
EXTRA = EXTRA
INS = INS
#endif /* __STDPP__ */

EDIT_OPTIONS = (ESH) (VSH) (RAWONLY) (ESHPLUS)
LIB_OPTIONS =  (KSHELL) (FLOAT) (MULTIBYTE) (PDU) (YELLOWP)
SH_OPTIONS = $(LIB_OPTIONS) (ACCT) (DBUG) (FLOAT) (JOBS) (MULIIBYTE) (OLDTEST)\
	(NEWTEST)  (SEVENBIT) (SUID_EXEC) (VFORK) $(EDIT_OPTIONS)

LDFLAGS = 		/* default loader options */
FILES_opt =		/* optional file list enabled by vars above */
LIBS_opt =		/* optional library list enabled by vars above */

#if machine(apollo)
FILES_opt += apollo.c
#endif

#if architecture(pdp11) || architecture(i8086)
LDFLAGS = -i
#endif

FILES_opt += $(ESH:T=V?emacs.c)
FILES_opt += $(VSH:T=V?vi.c)
FILES_opt += $(VFORK:T=V?vfork.c)

.ALL : $(SH) $(SUID_EXEC:T=V?suid_exec.c)
	rm -f sh_config.h

$(SH) :: arith.c args.c blok.c builtin.c cmd.c ctype.c defs.c \
	edit.c echo.c error.c expand.c fault.c history.c io.c jobs.c \
	macro.c main.c msg.c name.c print.c service.c string.c \
	stak.c test.c word.c xec.c $(FILES_opt) libsh.a $("libjobs.a":T=F) \
	$(LIBS_opt) $(SH_OPTIONS)

libsh.a :: adjust.c assign.c assnum.c cannon.c chkid.c convert.c \
	findnod.c gettree.c strmatch.c growaray.c gsort.c linknod.c \
	namscan.c rjust.c strdata.c streval.c tilde.c unassign.c utos.c  \
	valup.c optget.c $(LIB_OPTIONS)

#ifdef __STDPP__
:READONLY: msg.c ctype.c strdata.c
#else
msg.o ctype.o strdata.o : .READONLY
#endif /* __STDPP__ */

#ifdef __STDPP__
sh_config.h	:LINK: include/sh_config.h
#endif /* __STDPP__ */

include/sh_config.h:	config
	$(*) $(*:D) > newconfig 
	if	test -r $(<) &&  cmp -s newconfig $(<)
	then	rm newconfig	
	else	mv newconfig $(<)
	fi

libedit:	include/sh_config.h .ALWAYS
	if	test ! -d libedit
	then	mkdir libedit
	fi
	cd	libedit
	MAKEPATH=..${MAKEPATH+:}${MAKEPATH}
	export MAKEPATH
	if	${MAKE-/home/gsf/bin/nmake} libedit.a KSHELL= NEWTEST= \
			OLDTEST=  JOBS= SUID_EXEC=
	then	mv libedit.a ..
	fi
	
libedit.a :: edit.c editlib.c history.c $(ESH:T=V?emacs.c) $(VSH:T=V?vi.c) \
		$(SYSCALL) $(ED_OPTIONS)

suid_exec :: suid_exec.c

$(EXTRA):: syscall.s apollo.c jobs.c vfork.c ksh.bs makeshell makelibedit OPTIONS \
	gen_rdonly compile fcntl.c ksh.mk $(DOCFILES)

.ALWAYS:	.DONTCARE

$(INS):: config void.c int16.c data.c strchr.c token.c ispipe.c fastfork.c \
	vlimit.c void.c bcopy.c pgroup.c getppid.c setreuid.c sigrelse.c \
	sigrestart.c yellowp.c shellmagic wait3.c waitpid.c killpg.c \
	ftime.c opendir.c getgroups.c rand.c lstat.c getrlimit.c nfile.c \
	include/stak.h include/name.h include/shtype.h include/io.h include/edit.h \
	xmalloc.c proto.c setjmp.c select.c syscall.c path.c ioctl.c hz.c

