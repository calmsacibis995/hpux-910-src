BEHEAD=\
a.out.h \
ar.h \
assert.h \
audnetd.h \
bsdtty.h \
checklist.h \
cluster.h \
cpio.h \
ctype.h \
curses.h \
dbm.h \
debug.h \
devnm.h \
dial.h \
dirent.h \
disktab.h \
dl.h \
errno.h \
fcntl.h \
float.h \
ftw.h \
grp.h \
iconv.h \
langinfo.h \
limits.h \
locale.h \
magic.h \
malloc.h \
math.h \
memory.h \
mntent.h \
model.h \
msgbuf.h \
ndbm.h \
ndir.h \
net_diag.h \
netio.h \
nl_ctype.h \
nl_types.h \
nlist.h \
pwd.h \
regex.h \
regexp.h \
search.h \
setjmp.h \
sgtty.h \
shl.h \
signal.h \
stdarg.h \
stddef.h \
stdio.h \
stdlib.h \
string.h \
strings.h \
subsys_id.h \
symlink.h \
syslog.h \
switch.h \
tar.h \
term.h \
termio.h \
termios.h \
time.h \
ulimit.h \
unctrl.h \
unistd.h \
ustat.h \
utime.h \
utmp.h \
values.h \
varargs.h

NOBEHEAD=\
a.exec.h \
acllib.h \
alarm.h \
aouthdr.h \
aouttypes.h \
compunit.h \
core.h \
cvtnum.h \
dumprestor.h \
dvio.h \
elog.h \
execargs.h \
fatal.h \
fbackup.h \
filehdr.h \
fnmatch.h \
fstab.h \
glob.h \
globaldefs.h \
initptr.h \
iovalid.h \
kern_prof.h \
lst.h \
macros.h \
misc.h \
mon.h \
monetary.h \
msgcat.h \
mtab.h \
nan.h \
netdiag1.h \
oldfatal.h \
oldmacros.h \
poll.h \
port.h \
ranlib.h \
reloc.h \
rje.h \
scnhdr.h \
spacehdr.h \
stab.h \
symbol.h \
syms.h \
syscall.h \
sysexits.h \
sysinfo.h \
tcio.h \
timers.h \
tp_defs.h \
tryrec.h \
var.h \
volhdr.h \
wchar.h \
wordexp.h

JUSTCMDS=\
acl.h \
collate.h \
dnttsizes.h \
errnet.h \
fsg.h \
mandatory.h \
prot.h \
protcmd.h \
rquota.h \
setlocale.h \
symtab.h \
tp.h \
wpi.h

default:
		echo "done"

install:
		chmod 555 ./buildbe
		chmod 555 ./buildcinc
		chmod 555 ./buildcmds
		./buildbe $(BEHEAD)
		./buildcinc $(BEHEAD)
		./buildcinc $(NOBEHEAD)
		./buildcmds $(JUSTCMDS)
