/* @(#) $Revision: 66.3 $
 */

/*******************************************************************************
 *                    E V E N T M A P . H
 * maps auditable event or syscall numbers to names (or vice versa).
 * Note that the sc_map is a converged syscall table from the 300
 * and 800 syscalls. This is possible because of a characteristic
 * of the auditable syscalls: every syscall numbere maps unamiguously
 * to a syscall (except for syscall # 198, which needs special handling
 * in Audisp). This characteristic must be maintained for Audisp to
 * be able to translate syscalls.
 *
 * When the user asks Audisp to select on a syscall which has different
 * numbers on s300 and s800, Audisp remembers both numbers so that when
 * it sees either number in an audit record, that record will be displayed.
 *
 * There is an anomaly with syscall # 198. Since it maps to audswitch on
 * on s300 and vfsmount on s800, Audisp will not be able to tell whether
 * the user had asked for audswitch or vfsmount. Three things are added to
 * handle this anomaly:
 * (1) When the user specifies vfsmount, Audisp remembers 2 numbers: 234
 *     (for s300) and VFSMOUNT800 (for s800);
 * (2) When Audisp sees a syscall # 198, it determines (from the parameters)
 *     in the record whether it is vfsmount or audswitch. If it is audswitch,
 *     Audisp keeps the syscall # as 198; but if it is vfsmount, it changes
 *     the syscall # of the record to VFSMOUNT800.
 * (3) Therefore when it has to decide whether to display the record or not,
 *     it can use the syscall # and check if it is one of the syscalls
 *     the user had asked for.
 * Note that no special handling is needed when the user specifies audswitch.
 *     Audisp remembers 198 (for s300) and 247 (for s800).
 ******************************************************************************/

#define   EVMAPSIZE      evmapsize
#define   SCMAPSIZE      scmapsize
#define   VFSMOUNT800    1023 /* largest possible +ve int in 10 bits */
/*
 * event to name mapping
 */
struct ev_map {
   int ev_num;
   char *ev_name;
};

/*
 * syscall to name mapping
 */
struct sc_map {
   int sc_num;
   int ev_class;
   char *sc_name;
};

struct ev_map ev_map[] = {
   {   EN_CREATE,   "create" },
   {   EN_DELETE,   "delete" },
   {   EN_MODDAC,   "moddac" },
   {   EN_MODACCESS,   "modaccess" },
   {   EN_OPEN,   "open" },
   {   EN_CLOSE,   "close" },
   {   EN_PROCESS,   "process" },
   {   EN_REMOVABLE,   "removable" },
   {   EN_LOGIN,   "login" },
   {   EN_ADMIN,   "admin" },
   {   EN_IPCCREAT,   "ipccreat" },
   {   EN_IPCOPEN,   "ipcopen" },
   {   EN_IPCCLOSE,   "ipcclose" },
   {   EN_UEVENT1,   "uevent1" },
   {   EN_UEVENT2,   "uevent2" },
   {   EN_UEVENT3,   "uevent3" },
   {   EN_IPCDGRAM,  "ipcdgram" }
};

struct sc_map sc_map[] = {
   {   1,   EN_PROCESS,   "exit" },          /* syscalls # 1 - 169 are */
   {   2,   EN_PROCESS,   "fork" },          /* common to s300 and s800 */
   {   5,   EN_OPEN,   "open" },
   {   6,   EN_CLOSE,   "close" },
   {   8,   EN_CREATE,   "creat" },
   {   9,   EN_MODACCESS,   "link" },
   {   10,   EN_MODACCESS,   "unlink" },
   {   11,   EN_OPEN,   "execv" },
   {   12,   EN_MODACCESS,   "chdir" },
   {   14,   EN_CREATE,   "mknod" },
   {   15,   EN_MODDAC,   "chmod" },
   {   16,   EN_MODDAC,   "chown" },
   {   21,   EN_REMOVABLE,   "mount" },
   {   21,   EN_REMOVABLE,   "smount" },
   {   22,   EN_REMOVABLE,   "umount" },
   {   23,   EN_MODACCESS,   "setuid" },
   {   25,   EN_ADMIN,   "stime" },
   {   26,   EN_OPEN,   "ptrace" },
   {   37,   EN_PROCESS,   "kill" },
   {   42,   EN_CREATE,   "pipe" },
   {   46,   EN_MODACCESS,   "setgid" },
   {   55,   EN_ADMIN,   "reboot" },
   {   59,   EN_OPEN,   "execve" },
   {   60,   EN_MODDAC,   "umask" },
   {   61,   EN_MODACCESS,   "chroot" },
   {   66,   EN_PROCESS,   "vfork" },
   {   80,   EN_MODACCESS,   "setgroups" },
   {   85,   EN_ADMIN,   "swapon" },
   {   122,   EN_ADMIN,   "settimeofday" },
   {   123,   EN_MODDAC,   "fchown" },
   {   124,   EN_MODDAC,   "fchmod" },
   {   126,   EN_MODACCESS,   "setresuid" },
   {   127,   EN_MODACCESS,   "setresgid" },
   {   128,   EN_MODACCESS,   "rename" },
   {   129,   EN_OPEN,   "truncate" },
   {   130,   EN_OPEN,   "ftruncate" },
   {   136,   EN_CREATE,   "mkdir" },
   {   137,   EN_DELETE,   "rmdir" },
   {   143,   EN_ADMIN,   "sethostid" },
   {   151,   EN_ADMIN,   "privgrp" },
   {   151,   EN_ADMIN,   "setprivgrp" },
   {   156,   EN_CREATE,   "semget" },
   {   159,   EN_CREATE,   "msgget" },
   {   163,   EN_CREATE,   "shmget" },
   {   165,   EN_CREATE,   "shmat" },
   {   166,   EN_MODACCESS,   "shmdt" },
   {   168,   EN_PROCESS,   "nsp_init" },
   {   169,   EN_ADMIN,   "cluster" },

/* The following is a mixed list of s300 and s800 syscalls */

   {   186,   EN_MODDAC,   "setacl" },          /* s300 */
   {   187,   EN_MODDAC,   "fsetacl" },         /* s300 */
   {   192,   EN_ADMIN,   "setaudid" },         /* s300 */
   {   193,   EN_ADMIN,   "setdomainname" },    /* s800 */
   {   194,   EN_ADMIN,   "setaudproc" },       /* s300 */
   {   196,   EN_ADMIN,   "setevent" },         /* s300 */
   {   198,   EN_ADMIN,   "audswitch" },        /* s300 */
   {   199,   EN_ADMIN,   "audctl" },           /* s300 */
   {   234,   EN_REMOVABLE,   "vfsmount" },     /* s300 */
   {   236,   EN_ADMIN,   "setdomainname" },    /* s300 */
   {   237,   EN_MODDAC,   "setacl" },          /* s800 */
   {   238,   EN_MODDAC,   "fsetacl" },         /* s800 */
   {   241,   EN_ADMIN,   "setaudid" },         /* s800 */
   {   243,   EN_ADMIN,   "setaudproc" },       /* s800 */
   {   245,   EN_ADMIN,   "setevent" },         /* s800 */
   {   247,   EN_ADMIN,   "audswitch" },        /* s800 */
   {   248,   EN_ADMIN,   "audctl" },           /* s800 */
   {   275,   EN_IPCOPEN, "accept" },		/* common */
   {   276,   EN_IPCCREAT,   "bind" },		/* common */
   {   277,   EN_IPCOPEN, "connect" },		/* common */
   {   289,   EN_IPCCLOSE,   "shutdown" },	/* common */
   {   290,   EN_IPCCREAT,   "socket" },	/* common */
   {   298,   EN_IPCCREAT,   "ipccreate" },     /* common */
   {   301,   EN_IPCOPEN,   "ipclookup" },      /* common */
   {   303,   EN_IPCOPEN,   "ipcconnect" },     /* common */
   {   304,   EN_IPCOPEN,   "ipcrecvcn" },      /* common */
   {   310,   EN_IPCCLOSE,   "ipcshutdown" },   /* common */
   {   311,   EN_IPCCREAT,   "ipcdest" },       /* common */
   {   312,   EN_DELETE,   "semctl" },		/* common */
   {   313,   EN_DELETE,   "msgctl" },		/* common */
   {   314,   EN_MODACCESS,   "shmctl" },	/* common */
/* Special handling for syscall # 198 - see header comments. */
   {   VFSMOUNT800,   EN_REMOVABLE,   "vfsmount" }     /* s800 */
};

int         evmapsize = sizeof(ev_map)/sizeof(struct ev_map);
int         scmapsize = sizeof(sc_map)/sizeof(struct sc_map);
