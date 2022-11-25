# HPUX_ID: @(#) $Revision: 66.1 $ 
# makefile for ksh generated by mkold and hand modified
ROOT =
AR = ar
CC = cc
INC = $(ROOT)/usr/include
LD = ld
LDFLAGS = 
INS = install
SHLIB = shlib
CFLAGS = -O -I$(INC)
LIBS =
STRIP = strip
SIZE = size
BIN = $(ROOT)/bin
ETC = $(ROOT)/etc
Ins = install
Inc = include
Sh = sh
SH = ksh
RDONLY = $(Ins)/readonly


all:	$(SH) suid_exec

$(SH):	$(Inc)/sh_config.h arith.o args.o blok.o builtin.o cmd.o ctype.o \
		defs.o edit.o echo.o error.o expand.o fault.o history.o \
		io.o jobs.o macro.o main.o msg.o name.o print.o service.o \
		string.o stak.o test.o word.o xec.o emacs.o vi.o libsh.a
	$(CC) $(CFLAGS) -o $(SH) arith.o args.o blok.o builtin.o cmd.o \
		ctype.o defs.o edit.o echo.o error.o expand.o fault.o  \
		history.o io.o jobs.o macro.o main.o msg.o name.o print.o \
		service.o string.o stak.o test.o word.o xec.o emacs.o \
		vi.o libsh.a $(LIBS)
	-( mcs -d $(SH) ) 2> /dev/null

$(Inc)/sh_config.h:
		$(Ins)/config > newconfig
		if	test -r $(Inc)/sh_config.h && \
			cmp -s newconfig $(Inc)/sh_config.h;\
		then	rm newconfig	 ;\
		else	mv newconfig $(Inc)/sh_config.h;\
		fi

$(RDONLY):
		$(Ins)/gen_rdonly 2> /dev/null

arith.o:	$(Sh)/arith.c $(Inc)/streval.h $(Inc)/sh_config.h \
		$(Inc)/defs.h $(Inc)/stak.h $(Inc)/shnodes.h \
		$(Inc)/brkincr.h $(Inc)/name.h $(Inc)/flags.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/arith.c

args.o:	$(Sh)/args.c $(Inc)/builtins.h $(Inc)/sym.h $(Inc)/jobs.h \
		$(Inc)/terminal.h $(Inc)/defs.h $(Inc)/stak.h \
		$(Inc)/shnodes.h $(Inc)/brkincr.h $(Inc)/name.h \
		$(Inc)/flags.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/args.c

blok.o:	$(Sh)/blok.c $(Inc)/defs.h $(Inc)/stak.h $(Inc)/shnodes.h \
		$(Inc)/brkincr.h $(Inc)/name.h $(Inc)/flags.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/blok.c

builtin.o:	$(Sh)/builtin.c $(Inc)/sym.h $(Inc)/jobs.h \
		$(Inc)/terminal.h $(Inc)/builtins.h $(Inc)/history.h \
		$(Inc)/defs.h $(Inc)/stak.h $(Inc)/shnodes.h \
		$(Inc)/brkincr.h $(Inc)/name.h $(Inc)/flags.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/builtin.c

cmd.o:	$(Sh)/cmd.c $(Inc)/test.h $(Inc)/builtins.h $(Inc)/history.h \
		$(Inc)/sym.h $(Inc)/defs.h $(Inc)/stak.h \
		$(Inc)/shnodes.h $(Inc)/brkincr.h $(Inc)/name.h \
		$(Inc)/flags.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/cmd.c

ctype.o:	$(RDONLY) $(Sh)/ctype.c
	$(RDONLY) -I$(Inc) $(CFLAGS) -DKSHELL $(Sh)/ctype.c

defs.o:	$(Sh)/defs.c $(Inc)/timeout.h $(Inc)/edit.h $(Inc)/history.h \
		$(Inc)/sym.h $(Inc)/jobs.h $(Inc)/terminal.h \
		$(Inc)/defs.h $(Inc)/stak.h $(Inc)/shnodes.h \
		$(Inc)/brkincr.h $(Inc)/name.h $(Inc)/flags.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/defs.c

edit.o:	$(Sh)/edit.c $(Inc)/edit.h $(Inc)/history.h $(Inc)/sym.h \
		$(Inc)/builtins.h $(Inc)/terminal.h $(Inc)/defs.h \
		$(Inc)/stak.h $(Inc)/shnodes.h $(Inc)/brkincr.h \
		$(Inc)/name.h $(Inc)/flags.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/edit.c

echo.o:	$(Sh)/echo.c $(Inc)/defs.h $(Inc)/stak.h $(Inc)/shnodes.h \
		$(Inc)/brkincr.h $(Inc)/name.h $(Inc)/flags.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/echo.c

error.o:	$(Sh)/error.c $(Inc)/history.h $(Inc)/jobs.h \
		$(Inc)/terminal.h $(Inc)/defs.h $(Inc)/stak.h \
		$(Inc)/shnodes.h $(Inc)/brkincr.h $(Inc)/name.h \
		$(Inc)/flags.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/error.c

expand.o:	$(Sh)/expand.c $(Inc)/sh_config.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/expand.c

fault.o:	$(Sh)/fault.c $(Inc)/timeout.h $(Inc)/sym.h $(Inc)/jobs.h \
		$(Inc)/terminal.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/fault.c

history.o:	$(Sh)/history.c $(Inc)/history.h $(Inc)/builtins.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/history.c

io.o:	$(Sh)/io.c $(Inc)/history.h $(Inc)/sym.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/io.c

jobs.o:	$(Sh)/jobs.c $(Inc)/history.h $(Inc)/jobs.h $(Inc)/terminal.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/jobs.c

macro.o:	$(Sh)/macro.c $(Inc)/builtins.h $(Inc)/sym.h $(Inc)/terminal.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/macro.c

main.o:	$(Sh)/main.c $(Inc)/builtins.h $(Inc)/timeout.h \
		$(Inc)/history.h $(Inc)/sym.h $(Inc)/jobs.h \
		$(Inc)/terminal.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/main.c

msg.o:	$(RDONLY) $(Sh)/msg.c $(Inc)/history.h $(Inc)/timeout.h $(Inc)/test.h \
		$(Inc)/builtins.h $(Inc)/sym.h
	$(RDONLY) $(CFLAGS) -I$(Inc) -DKSHELL $(Sh)/msg.c

name.o:	$(Sh)/name.c $(Inc)/timeout.h $(Inc)/history.h \
		$(Inc)/builtins.h $(Inc)/sym.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/name.c

print.o:	$(Sh)/print.c $(Inc)/builtins.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/print.c

service.o:	$(Sh)/service.c $(Inc)/history.h $(Inc)/builtins.h \
		$(Inc)/sym.h $(Inc)/jobs.h $(Inc)/terminal.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/service.c

string.o:	$(Sh)/string.c $(Inc)/sym.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/string.c

stak.o:	$(Sh)/stak.c
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/stak.c

test.o:	$(Sh)/test.c $(Inc)/sym.h $(Inc)/test.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/test.c

word.o:	$(Sh)/word.c $(Inc)/test.h $(Inc)/builtins.h $(Inc)/sym.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/word.c

xec.o:	$(Sh)/xec.c $(Inc)/builtins.h $(Inc)/test.h $(Inc)/sym.h \
		$(Inc)/jobs.h $(Inc)/terminal.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/xec.c

emacs.o:	$(Sh)/emacs.c $(Inc)/edit.h $(Inc)/history.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/emacs.c

vi.o:	$(Sh)/vi.c $(Inc)/terminal.h $(Inc)/edit.h $(Inc)/history.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(Sh)/vi.c

libsh.a:	adjust.o assign.o assnum.o cannon.o chkid.o convert.o \
		findnod.o gettree.o strmatch.o growaray.o gsort.o \
		linknod.o namscan.o rjust.o strdata.o streval.o tilde.o \
		unassign.o utos.o valup.o optget.o
	$(AR) cr libsh.a adjust.o assign.o assnum.o cannon.o chkid.o \
		convert.o findnod.o gettree.o strmatch.o growaray.o gsort.o \
		linknod.o namscan.o rjust.o strdata.o streval.o tilde.o \
		unassign.o utos.o valup.o optget.o
		-(ranlib libsh.a) > /dev/null 2>&1

adjust.o:	$(SHLIB)/adjust.c $(Inc)/name.h $(Inc)/flags.h $(Inc)/sh_config.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/adjust.c

assign.o:	$(SHLIB)/assign.c $(Inc)/name.h $(Inc)/flags.h $(Inc)/sh_config.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/assign.c

assnum.o:	$(SHLIB)/assnum.c $(Inc)/name.h $(Inc)/flags.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/assnum.c

cannon.o:	$(SHLIB)/cannon.c $(Inc)/sh_config.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/cannon.c

chkid.o:	$(SHLIB)/chkid.c $(Inc)/sh_config.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/chkid.c

convert.o:	$(SHLIB)/convert.c
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/convert.c

findnod.o:	$(SHLIB)/findnod.c $(Inc)/name.h $(Inc)/flags.h $(Inc)/sh_config.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/findnod.c

gettree.o:	$(SHLIB)/gettree.c $(Inc)/name.h $(Inc)/flags.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/gettree.c

strmatch.o:	$(SHLIB)/strmatch.c
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/strmatch.c

growaray.o:	$(SHLIB)/growaray.c $(Inc)/name.h $(Inc)/flags.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/growaray.c

gsort.o:	$(SHLIB)/gsort.c
	$(CC) -I$(Inc) $(CFLAGS) -c $(SHLIB)/gsort.c

linknod.o:	$(SHLIB)/linknod.c $(Inc)/name.h $(Inc)/flags.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/linknod.c

namscan.o:	$(SHLIB)/namscan.c $(Inc)/name.h $(Inc)/flags.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/namscan.c

rjust.o:	$(SHLIB)/rjust.c
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/rjust.c

strdata.o:	$(RDONLY) $(SHLIB)/strdata.c $(Inc)/streval.h $(Inc)/sh_config.h
	$(RDONLY) $(CFLAGS) -I$(Inc) -DKSHELL $(SHLIB)/strdata.c

streval.o:	$(SHLIB)/streval.c $(Inc)/streval.h $(Inc)/sh_config.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/streval.c

tilde.o:	$(SHLIB)/tilde.c $(Inc)/sh_config.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/tilde.c

unassign.o:	$(SHLIB)/unassign.c $(Inc)/name.h $(Inc)/flags.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/unassign.c

utos.o:	$(SHLIB)/utos.c
	$(CC) -I$(Inc) $(CFLAGS) -c $(SHLIB)/utos.c

valup.o:	$(SHLIB)/valup.c $(Inc)/name.h $(Inc)/flags.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/valup.c

optget.o:	$(SHLIB)/optget.c $(Inc)/sh_config.h
	$(CC) -I$(Inc) $(CFLAGS) -DKSHELL -c $(SHLIB)/optget.c

suid_exec:	suid_exec.o
	$(CC) $(CFLAGS) -o suid_exec suid_exec.o

suid_exec.o :	$(Sh)/suid_exec.c $(Inc)/sh_config.h
	$(CC) -I$(Inc) $(CFLAGS) -c $(Sh)/suid_exec.c

clobber :	clean
		rm -f ./$(SH) $(Inc)/sh_config.h libsh.a core

clean :
	rm -f *.o

install : strip
	-rm -f $(BIN)/$(SH)
	$(INS) -o -m 555 -u bin -g root -f $(BIN) $(SH)
	-rm -f $(ETC)/suid_exec
	$(INS) -o -m 4511 -u root -g root -f $(ETC) suid_exec
	rm -f $(BIN)/r$(SH)
	ln -f $(BIN)/$(SH) $(BIN)/r$(SH)
	rm -f $(SH) suid_exec

size: all
	$(SIZE)  $(SH) suid_exec

strip: all
	$(STRIP)  $(SH) suid_exec

