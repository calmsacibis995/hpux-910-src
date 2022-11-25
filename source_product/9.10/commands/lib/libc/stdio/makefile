# $Source: /misc/source_product/9.10/commands.rcs/lib/libc/stdio/makefile,v $
# @(#) $Revision: 70.5 $

# Common makefile for libc/stdio subdirectory
# Description file for stdio: io package

OBJS =			\
	bufsync.o	\
	cleanup.o	\
	ctermid.o	\
	cuserid.o	\
	data.o		\
	doprnt.o	\
	doscan.o	\
	exitcu.o	\
	fdopen.o	\
	ferror.o	\
	fflush.o	\
	fgetc.o		\
	fgetpos.o	\
	fgets.o		\
	filbuf.o	\
	findiop.o	\
	flsbuf.o	\
	fopen.o		\
	fprintf.o	\
	fputc.o		\
	fputs.o		\
	fread.o		\
	fseek.o		\
	fsetpos.o	\
	ftell.o		\
	fwrite.o	\
	get_fd_FILE.o	\
	getchar.o	\
	getpass.o	\
	gets.o		\
	getw.o		\
	nl_fprintf.o	\
	nl_printf.o	\
	nl_scan.o	\
	nl_scanf.o	\
	nl_sprintf.o	\
	popen.o		\
	printf.o	\
	putchar.o	\
	puts.o		\
	putw.o		\
	rew.o		\
	scanf.o		\
	scan_iops.o	\
	setbuf.o	\
	setvbuf.o	\
	sprintf.o	\
	system.o	\
	tempnam.o	\
	tmpfile.o	\
	tmpnam.o	\
	ungetc.o	\
	vprintf.o	\
	vscanf.o	\
	vfprintf.o	\
	vsprintf.o      \
	xflsbuf.o

OBJS800 = blockmode.o

OBJS_P = $(OBJS:.o=_p.o)
OBJS800_P = $(OBJS800:.o=_p.o)

OBJS_SL = $(OBJS:.o=_sl.o)
OBJS800_SL = $(OBJS800:.o=_sl.o)

OBJS_PIC = $(OBJS:.o=_pic.o)
OBJS800_PIC = $(OBJS800:.o=_pic.o)

default:	archive

archive:
	if hp9000s300; then					\
		objs="$(OBJS)";					\
	elif hp9000s800; then					\
		objs="$(OBJS) $(OBJS800)";			\
	fi;							\
	$(MAKE) OBJS="$$objs" DIR=tmp stdio

shared:
	if hp9000s300; then					\
		objs="$(OBJS_SL)";				\
	elif hp9000s800; then					\
		objs="$(OBJS_SL) $(OBJS800_SL)";		\
	fi;							\
	$(MAKE) OBJS="$$objs" DIR=tmp_sl stdio

profile:
	if hp9000s300; then					\
		objs="$(OBJS_P)";				\
	elif hp9000s800; then					\
		objs="$(OBJS_P) $(OBJS800_P)";			\
	fi;							\
	$(MAKE) OBJS="$$objs" DIR=tmp_p stdio

archive_pic:
	if hp9000s300; then					\
		objs="$(OBJS_PIC)";				\
	elif hp9000s800; then					\
		objs="$(OBJS_PIC) $(OBJS800_PIC)";		\
	fi;							\
	$(MAKE) OBJS="$$objs" DIR=tmp_pic stdio

stdio: $(OBJS)
	@echo "\tLinking objects to ../$(DIR)..."
	@if [ ! -d ../$(DIR) ]; then		\
	    mkdir ../$(DIR) >/dev/null 2>&1;	\
	fi;					\
	exit 0
	@ln $(OBJS) ../$(DIR)
	touch ../$(DIR)/OK.$@

clean:
	/bin/rm -f $(OBJS) $(OBJS800) \
		   $(OBJS_P) $(OBJS800_P) \
		   $(OBJS_SL) $(OBJS800_SL) \
		   $(OBJS_PIC) $(OBJS800_PIC)

clobber:
	@echo "Clobbering is done by the top-level libc makefile"

#
# This explicit doprnt rule is for the sake of the s300, which had an
# assembly version previously.
# The reason for two different rules is that on the s300, only level 1
# optimization is used to ensure floating point compatibility with pre-6.5
# object files.
# NOTE: the +O1 must come *after* the $(CFLAGS) (as long as CFLAGS is -O
# anyway) as the last option encountered takes precedence.
#
doprnt.o:	doprnt.c
	if hp9000s300; then				\
		$(CC) $(CFLAGS) +O1 -c doprnt.c -o $@ ;	\
	elif hp9000s800; then				\
		$(CC) $(CFLAGS) +Obb1000 -c doprnt.c -o $@ ;	\
	fi;

doprnt_p.o:	doprnt.c
	if hp9000s300; then				\
		$(CC) $(CFLAGS) $(PRFLAGS) +O1 -c doprnt.c -o $@ ;	\
	elif hp9000s800; then				\
		$(CC) $(CFLAGS) $(PRFLAGS) +Obb1000 -c doprnt.c -o $@ ;	\
	fi;

doprnt_sl.o:	doprnt.c
	if hp9000s300; then				\
		$(CC) $(CFLAGS) +O1 -c doprnt.c -o $@ ;	\
	elif hp9000s800; then				\
		$(CC) $(CFLAGS) +Obb1000 -c doprnt.c -o $@ ;	\
	fi;

doprnt_pic.o:	doprnt.c
	if hp9000s300; then				\
		$(CC) $(CFLAGS) +O1 -c doprnt.c -o $@ ;	\
	elif hp9000s800; then				\
		$(CC) $(CFLAGS) +Obb1000 -c doprnt.c -o $@ ;	\
	fi;

#
# Make a special version of doprnt that doesn't have the quad float
# support in it.  This is just until we have shared libraries.
# (or maybe longer since we will still link with archive versions
# for several commands)
#
doprnt_bld.o:	doprnt.c
	if hp9000s300; then					     \
	    $(CC) $(CFLAGS) -DNO_LONG_DOUBLE +O1 -c doprnt.c -o $@;  \
	elif hp9000s800; then					     \
	    $(CC) $(CFLAGS) -DNO_LONG_DOUBLE +Obb1000 -c doprnt.c -o $@;      \
	fi;

#
# Define rules so make knows how to assemble/compile normal and profiled .o's
#
.s.o:
		$(M4) $(M4FLAGS) $< | $(AS) $(ASFLAGS) -o $@

.s_p.o:
		$(M4) $(M4FLAGS) $< | $(AS) $(ASFLG_P) -o $@

.s_sl.o:
		$(M4) $(M4FLAGS) $< | $(AS) $(ASFLAGS) -o $@

.s_pic.o:
		$(M4) $(M4FLAGS) $< | $(AS) $(ASFLAGS) -o $@

.c_p.o:
	$(CC) $(CFLAGS) $(PRFLAGS) -c $< -o $@

.c_sl.o:
	$(CC) $(CFLAGS) -c $< -o $@

.c_pic.o:
	$(CC) $(CFLAGS) -c $< -o $@

#
# clear SUFFIXES, then reset ordering so if a .c and a .s source exist for
# a file, the .s will be compiled first
#
.SUFFIXES:
.SUFFIXES: .o .s .c _p.o _sl.o _pic.o
