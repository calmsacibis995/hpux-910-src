/*
 * @(#) $Revision: 66.3 $
 */

struct aud_event_tbl	ev_tab;
struct aud_type		sc_tab[MAX_SYSCALL+1];

int			fail = -1;
int			pass = -1;

/*
 * it would be nice if syscalls and events had the
 * same structure. the event table is currently a
 * single structure; the syscall table is an array
 * of single-element structures.
 *
 * ev_map is set up so that it might be more easily
 * merged with sc_map if and when it does occur.
 */
struct ev_map ev_map[] = {
	{	0,		EN_CREATE,	"create" },
	{	0,		EN_DELETE,	"delete" },
	{	0,		EN_MODDAC,	"moddac" },
	{	0,		EN_MODACCESS,	"modaccess" },
	{	0,		EN_OPEN,	"open" },
	{	0,		EN_CLOSE,	"close" },
	{	0,		EN_PROCESS,	"process" },
	{	0,		EN_REMOVABLE,	"removable" },
	{	0,		EN_LOGIN,	"login" },
	{	0,		EN_ADMIN,	"admin" },
	{	0,		EN_IPCCREAT,	"ipccreat" },
	{	0,		EN_IPCOPEN,	"ipcopen" },
	{	0,		EN_IPCCLOSE,	"ipcclose" },
	{	0,		EN_UEVENT1,	"uevent1" },
	{	0,		EN_UEVENT2,	"uevent2" },
	{	0,		EN_UEVENT3,	"uevent3" },
	{	0,		EN_IPCDGRAM,	"ipcdgram" }
};

/*
 * second field will probably be different for s300.
 * do wholesale ifdeffing to make it cleaner.
 */
struct sc_map sc_map[] = {
#ifdef hp9000s300
	{	0,	1,	EN_PROCESS,	"exit" },
	{	0,	2,	EN_PROCESS,	"fork" },
	{	0,	5,	EN_OPEN,	"open" },
	{	0,	6,	EN_CLOSE,	"close" },
	{	0,	8,	EN_CREATE,	"creat" },
	{	0,	9,	EN_MODACCESS,	"link" },
	{	0,	10,	EN_MODACCESS,	"unlink" },
	{	0,	11,	EN_OPEN,	"execv" },
	{	0,	12,	EN_MODACCESS,	"chdir" },
	{	0,	14,	EN_CREATE,	"mknod" },
	{	0,	15,	EN_MODDAC,	"chmod" },
	{	0,	16,	EN_MODDAC,	"chown" },
	{	0,	21,	EN_REMOVABLE,	"mount" },
	{	0,	22,	EN_REMOVABLE,	"umount" },
	{	0,	23,	EN_MODACCESS,	"setuid" },
	{	0,	25,	EN_ADMIN,	"stime" },
	{	0,	26,	EN_OPEN,	"ptrace" },
	{	0,	37,	EN_PROCESS,	"kill" },
	{	0,	42,	EN_CREATE,	"pipe" },
	{	0,	46,	EN_MODACCESS,	"setgid" },
	{	0,	55,	EN_ADMIN,	"reboot" },
	{	0,	59,	EN_OPEN,	"execve" },
	{	0,	60,	EN_MODDAC,	"umask" },
	{	0,	61,	EN_MODACCESS,	"chroot" },
	{	0,	66,	EN_PROCESS,	"vfork" },
	{	0,	80,	EN_MODACCESS,	"setgroups" },
	{	0,	85,	EN_ADMIN,	"swapon" },
	{	0,	122,	EN_ADMIN,	"settimeofday" },
	{	0,	123,	EN_MODDAC,	"fchown" },
	{	0,	124,	EN_MODDAC,	"fchmod" },
	{	0,	126,	EN_MODACCESS,	"setresuid" },
	{	0,	127,	EN_MODACCESS,	"setresgid" },
	{	0,	128,	EN_MODACCESS,	"rename" },
	{	0,	129,	EN_OPEN,	"truncate" },
	{	0,	130,	EN_OPEN,	"ftruncate" },
	{	0,	136,	EN_CREATE,	"mkdir" },
	{	0,	137,	EN_DELETE,	"rmdir" },
	{	0,	143,	EN_ADMIN,	"sethostid" },
	{	0,	151,	EN_ADMIN,	"setprivgrp" },
	{	0,	156,	EN_CREATE,	"semget" },
	{	0,	159,	EN_CREATE,	"msgget" },
	{	0,	163,	EN_CREATE,	"shmget" },
	{	0,	165,	EN_CREATE,	"shmat" },
	{	0,	166,	EN_MODACCESS,	"shmdt" },
	{	0,	168,	EN_PROCESS,	"nsp_init" },
	{	0,	169,	EN_ADMIN,	"cluster" },
	{	0,	186,	EN_MODDAC,	"setacl" },
	{	0,	187,	EN_MODDAC,	"fsetacl" },
	{	0,	192,	EN_ADMIN,	"setaudid" },
	{	0,	194,	EN_ADMIN,	"setaudproc" },
	{	0,	196,	EN_ADMIN,	"setevent" },
	{	0,	198,	EN_ADMIN,	"audswitch" },
	{	0,	199,	EN_ADMIN,	"audctl" },
	{	0,	234,	EN_REMOVABLE,	"vfsmount" },
	{	0,	236,	EN_ADMIN,	"setdomainname" },
	{	0,	275,	EN_IPCOPEN,	"accept" },
	{	0,	276,	EN_IPCCREAT,	"bind" },
	{	0,	277,	EN_IPCOPEN,	"connect" },
	{	0,	289,	EN_IPCCLOSE,	"shutdown" },
	{	0,	290,	EN_IPCCREAT,	"socket" },
	{	0,	298,	EN_IPCCREAT,	"ipccreate" },
	{	0,	301,	EN_IPCOPEN,	"ipclookup" },
	{	0,	303,	EN_IPCOPEN,	"ipcconnect" },
	{	0,	304,	EN_IPCOPEN,	"ipcrecvcn" },
	{	0,	310,	EN_IPCCLOSE,	"ipcshutdown" },
	{	0,	311,	EN_IPCCREAT,	"ipcdest" },
	{	0,	312,	EN_DELETE,	"semctl" },
	{	0,	313,	EN_DELETE,	"msgctl" },
	{	0,	314,	EN_MODACCESS,	"shmctl" }
#endif /* hp9000s300 */
#ifdef hp9000s800
	{	0,	1,	EN_PROCESS,	"exit" },
	{	0,	2,	EN_PROCESS,	"fork" },
	{	0,	5,	EN_OPEN,	"open" },
	{	0,	6,	EN_CLOSE,	"close" },
	{	0,	8,	EN_CREATE,	"creat" },
	{	0,	9,	EN_MODACCESS,	"link" },
	{	0,	10,	EN_MODACCESS,	"unlink" },
	{	0,	11,	EN_OPEN,	"execv" },
	{	0,	12,	EN_MODACCESS,	"chdir" },
	{	0,	14,	EN_CREATE,	"mknod" },
	{	0,	15,	EN_MODDAC,	"chmod" },
	{	0,	16,	EN_MODDAC,	"chown" },
	{	0,	21,	EN_REMOVABLE,	"mount" },
	{	0,	22,	EN_REMOVABLE,	"umount" },
	{	0,	23,	EN_MODACCESS,	"setuid" },
	{	0,	25,	EN_ADMIN,	"stime" },
	{	0,	26,	EN_OPEN,	"ptrace" },
	{	0,	37,	EN_PROCESS,	"kill" },
	{	0,	42,	EN_CREATE,	"pipe" },
	{	0,	46,	EN_MODACCESS,	"setgid" },
	{	0,	55,	EN_ADMIN,	"reboot" },
	{	0,	59,	EN_OPEN,	"execve" },
	{	0,	60,	EN_MODDAC,	"umask" },
	{	0,	61,	EN_MODACCESS,	"chroot" },
	{	0,	66,	EN_PROCESS,	"vfork" },
	{	0,	80,	EN_MODACCESS,	"setgroups" },
	{	0,	85,	EN_ADMIN,	"swapon" },
	{	0,	122,	EN_ADMIN,	"settimeofday" },
	{	0,	123,	EN_MODDAC,	"fchown" },
	{	0,	124,	EN_MODDAC,	"fchmod" },
	{	0,	126,	EN_MODACCESS,	"setresuid" },
	{	0,	127,	EN_MODACCESS,	"setresgid" },
	{	0,	128,	EN_MODACCESS,	"rename" },
	{	0,	129,	EN_OPEN,	"truncate" },
	{	0,	130,	EN_OPEN,	"ftruncate" },
	{	0,	136,	EN_CREATE,	"mkdir" },
	{	0,	137,	EN_DELETE,	"rmdir" },
	{	0,	143,	EN_ADMIN,	"sethostid" },
	{	0,	151,	EN_ADMIN,	"setprivgrp" },
	{	0,	156,	EN_CREATE,	"semget" },
	{	0,	159,	EN_CREATE,	"msgget" },
	{	0,	163,	EN_CREATE,	"shmget" },
	{	0,	165,	EN_CREATE,	"shmat" },
	{	0,	166,	EN_MODACCESS,	"shmdt" },
	{	0,	168,	EN_PROCESS,	"nsp_init" },
	{	0,	169,	EN_ADMIN,	"cluster" },
	{	0,	193,	EN_ADMIN,	"setdomainname" },
	{	0,	198,	EN_REMOVABLE,	"vfsmount" },
	{	0,	237,	EN_MODDAC,	"setacl" },
	{	0,	238,	EN_MODDAC,	"fsetacl" },
	{	0,	241,	EN_ADMIN,	"setaudid" },
	{	0,	243,	EN_ADMIN,	"setaudproc" },
	{	0,	245,	EN_ADMIN,	"setevent" },
	{	0,	247,	EN_ADMIN,	"audswitch" },
	{	0,	248,	EN_ADMIN,	"audctl" },
	{	0,	275,	EN_IPCOPEN,	"accept" },
	{	0,	276,	EN_IPCCREAT,	"bind" },
	{	0,	277,	EN_IPCOPEN,	"connect" },
	{	0,	289,	EN_IPCCLOSE,	"shutdown" },
	{	0,	290,	EN_IPCCREAT,	"socket" },
	{	0,	298,	EN_IPCCREAT,	"ipccreate" },
	{	0,	301,	EN_IPCOPEN,	"ipclookup" },
	{	0,	303,	EN_IPCOPEN,	"ipcconnect" },
	{	0,	304,	EN_IPCOPEN,	"ipcrecvcn" },
	{	0,	310,	EN_IPCCLOSE,	"ipcshutdown" },
	{	0,	311,	EN_IPCCREAT,	"ipcdest" },
	{	0,	312,	EN_DELETE,	"semctl" },
	{	0,	313,	EN_DELETE,	"msgctl" },
	{	0,	314,	EN_MODACCESS,	"shmctl" }
#endif /* hp9000s800 */
};

int			evmapsize = sizeof(ev_map)/sizeof(struct ev_map);
int			scmapsize = sizeof(sc_map)/sizeof(struct sc_map);
