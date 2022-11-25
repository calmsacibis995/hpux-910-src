/* @(#) $Revision: 70.3 $ */
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#   define sys_errlist _sys_errlist
#   define sys_nerr _sys_nerr
#endif

/* The UNKNOWN string is defined here, so that the only space wasted  */
/* for each "placeholder" error is the 4 bytes for the char * pointer */

/********************************************************************
** The catgets number for UNKNOWN *must* be 1 more than
** the catgets number of the last message in sys_errlist.
** For example, if the catgets number of the last message is 252
** then the catgets number for UNKNOWN is 253.
*********************************************************************/

static	char	UNKNOWN[]	= "Unknown error"; 	/* catgets 253 */

#ifdef _NAMESPACE_CLEAN
#   undef sys_errlist
#   pragma _HP_SECONDARY_DEF _sys_errlist sys_errlist
#   define sys_errlist _sys_errlist
#endif

/********************************************************************
 *  NOTE WHEN ADDING NEW ERRORS THAT THE NUMBER FOLLOWING "catgets" *
 *  MUST BE 1 GREATER THAN THE ACTUAL ERROR NUMBER.                 *
 *  IF NEW ERRORS ARE ADDED AT THE END OF SYS_ERRLIST,              *
 *  THEN THE "catgets" NUMBER OF UNKNOWN (SEE ABOVE)                *
 *  MUST BE UPDATED.                                                *
 ********************************************************************/

char	*sys_errlist[] = {
	"Error 0",				/* 0		catgets 1 */
	"Not owner",                            /* 1 - EPERM 	catgets 2 */
	"No such file or directory",            /* 2 - ENOENT	catgets 3 */
	"No such process",                      /* 3 - ESRCH	catgets 4 */
	"Interrupted system call",              /* 4 - EINTR	catgets 5 */
	"I/O error",                            /* 5 - EIO	catgets 6 */
	"No such device or address",            /* 6 - ENXIO	catgets 7 */
	"Arg list too long",                    /* 7 - E2BIG	catgets 8 */
	"Exec format error",                    /* 8 - ENOEXEC	catgets 9 */
	"Bad file number",                      /* 9 - EBADF	catgets 10 */
	"No child processes",                   /* 10 - ECHILD	catgets 11 */
	"No more processes",                    /* 11 - EAGAIN	catgets 12 */
	"Not enough space",                     /* 12 - ENOMEM	catgets 13 */
	"Permission denied",                    /* 13 - EACCES	catgets 14 */
	"Bad address",                          /* 14 - EFAULT	catgets 15 */
	"Block device required",                /* 15 - ENOTBLK	catgets 16 */
	"Device busy",                          /* 16 - EBUSY	catgets 17 */
	"File exists",                          /* 17 - EEXIST	catgets 18 */
	"Cross-device link",                    /* 18 - EXDEV	catgets 19 */
	"No such device",                       /* 19 - ENODEV	catgets 20 */
	"Not a directory",                      /* 20 - ENOTDIR	catgets 21 */
	"Is a directory",                       /* 21 - EISDIR	catgets 22 */
	"Invalid argument",                     /* 22 - EINVAL	catgets 23 */
	"File table overflow",                  /* 23 - ENFILE	catgets 24 */
	"Too many open files",                  /* 24 - EMFILE	catgets 25 */
	"Not a typewriter",                     /* 25 - ENOTTY	catgets 26 */
	"Text file busy",                       /* 26 - ETXTBSY	catgets 27 */
	"File too large",                       /* 27 - EFBIG	catgets 28 */
	"No space left on device",              /* 28 - ENOSPC	catgets 29 */
	"Illegal seek",                         /* 29 - ESPIPE	catgets 30 */
	"Read-only file system",                /* 30 - EROFS	catgets 31 */
	"Too many links",                       /* 31 - EMLINK	catgets 32 */
	"Broken pipe",                          /* 32 - EPIPE	catgets 33 */

/* math software */
	"Argument out of domain",               /* 33 - EDOM	catgets 34 */
	"Result too large",			/* 34 - ERANGE	catgets 35 */


/* System V IPC errors */
	"No message of desired type",           /* 35 - ENOMSG	catgets 36 */
	"Identifier removed",			/* 36 - EIDRM	catgets 37 */

/* defined for Bell only */
	UNKNOWN,				/* 37 - undefined	"Unknown error" catgets 38 */
	UNKNOWN,				/* 38 - undefined	"Unknown error" catgets 39 */
	UNKNOWN,				/* 39 - undefined	"Unknown error" catgets 40 */
	UNKNOWN,				/* 40 - undefined	"Unknown error" catgets 41 */
	UNKNOWN,				/* 41 - undefined	"Unknown error" catgets 42 */
	UNKNOWN,				/* 42 - undefined	"Unknown error" catgets 43 */
	UNKNOWN,				/* 43 - undefined	"Unknown error" catgets 44 */
	UNKNOWN,				/* 44 - undefined	"Unknown error" catgets 45 */
/* LOCKF errors */
	"Lockf deadlock detection",		/* 45 - EDEADLK	catgets 46 */
	"No locks available",			/* 46 - ENOLCK	catgets 47 */

/* filler, stream errors */
	"Illegal byte sequence",		/* 47 - EILSEQ	 catgets 48 */
	UNKNOWN,				/* 48 - undefined	"Unknown error" catgets 49 */
	UNKNOWN,				/* 49 - undefined	"Unknown error" catgets 50 */
	"Not a stream",				/* 50 - ENOSTR	 catgets 51 */
	"No data",				/* 51 - ENODATA	 catgets 52 */
	"Stream ioctl timeout",			/* 52 - ETIME	 catgets 53 */
	"No stream resources",			/* 53 - ENOSR	 catgets 54 */
	"Not on the network",			/* 54 - ENONET	 catgets 55 */
	"Package not installed",		/* 55 - ENOPKG	 catgets 56 */
	UNKNOWN,				/* 56 - undefined	"Unknown error" catgets 57 */
	"Link severed",				/* 57 - ENOLINK	 catgets 58 */
	"Advertise error",			/* 58 - EADV	 catgets 59 */
	"Srmount error",			/* 59 - ESRMNT	 catgets 60 */
	"Error on send",			/* 60 - ECOMM	 catgets 61 */
	"Protocol error",			/* 61 - EPROTO	 catgets 62 */
	UNKNOWN,				/* 62 - undefined	"Unknown error" catgets 63 */
	UNKNOWN,				/* 63 - undefined	"Unknown error" catgets 64 */
	"Multihop attempted",			/* 64 - EMULTIHOP catgets 65 */
	UNKNOWN,				/* 65 - undefined	"Unknown error" catgets 66 */
	"Cross mount point",			/* 66 - EDOTDOT	 catgets 67 */
	"Bad message",				/* 67 - EBADMSG	 catgets 68 */
	UNKNOWN,				/* 68 - EUSERS	 catgets 69 */
	"Disk quota exceeded",			/* 69 - EDQUOT	 catgets 70 */
	"Stale NFS file handle",		/* 70 - ESTALE	catgets 71 */
	"Too many levels of remote in path",	/* 71 - EREMOTE	catgets 72 */
	UNKNOWN,				/* 72 - undefined	"Unknown error" catgets 73 */
	UNKNOWN,				/* 73 - undefined	"Unknown error" catgets 74 */
	UNKNOWN,				/* 74 - undefined	"Unknown error" catgets 75 */
	UNKNOWN,				/* 75 - undefined	"Unknown error" catgets 76 */
	UNKNOWN,				/* 76 - undefined	"Unknown error" catgets 77 */
	UNKNOWN,				/* 77 - undefined	"Unknown error" catgets 78 */
	UNKNOWN,				/* 78 - undefined	"Unknown error" catgets 79 */
	UNKNOWN,				/* 79 - undefined	"Unknown error" catgets 80 */
	UNKNOWN,				/* 80 - undefined	"Unknown error" catgets 81 */
	UNKNOWN,				/* 81 - undefined	"Unknown error" catgets 82 */
	UNKNOWN,				/* 82 - undefined	"Unknown error" catgets 83 */
	UNKNOWN,				/* 83 - undefined	"Unknown error" catgets 84 */
	UNKNOWN,				/* 84 - undefined	"Unknown error" catgets 85 */
	UNKNOWN,				/* 85 - undefined	"Unknown error" catgets 86 */
	UNKNOWN,				/* 86 - undefined	"Unknown error" catgets 87 */
	UNKNOWN,				/* 87 - undefined	"Unknown error" catgets 88 */
	UNKNOWN,				/* 88 - undefined	"Unknown error" catgets 89 */
	UNKNOWN,				/* 89 - undefined	"Unknown error" catgets 90 */
	UNKNOWN,				/* 90 - undefined	"Unknown error" catgets 91 */
	UNKNOWN,				/* 91 - undefined	"Unknown error" catgets 92 */
	UNKNOWN,				/* 92 - undefined	"Unknown error" catgets 93 */
	UNKNOWN,				/* 93 - undefined	"Unknown error" catgets 94 */
	UNKNOWN,				/* 94 - undefined	"Unknown error" catgets 95 */
	UNKNOWN,				/* 95 - undefined	"Unknown error" catgets 96 */
	UNKNOWN,				/* 96 - undefined	"Unknown error" catgets 97 */
	UNKNOWN,				/* 97 - undefined	"Unknown error" catgets 98 */
	UNKNOWN,				/* 98 - undefined	"Unknown error" catgets 99 */
	"Unexpected Error",			/* 99 - EUNEXPECT catgets 100 */
	UNKNOWN,				/* 100 - undefined	"Unknown error" catgets 101 */
	UNKNOWN,				/* 101 - undefined	"Unknown error" catgets 102 */
	UNKNOWN,				/* 102 - undefined	"Unknown error" catgets 103 */
	UNKNOWN,				/* 103 - undefined	"Unknown error" catgets 104 */
	UNKNOWN,				/* 104 - undefined	"Unknown error" catgets 105 */
	UNKNOWN,				/* 105 - undefined	"Unknown error" catgets 106 */
	UNKNOWN,				/* 106 - undefined	"Unknown error" catgets 107 */
	UNKNOWN,				/* 107 - undefined	"Unknown error" catgets 108 */
	UNKNOWN,				/* 108 - undefined	"Unknown error" catgets 109 */
	UNKNOWN,				/* 109 - undefined	"Unknown error" catgets 110 */
	UNKNOWN,				/* 110 - undefined	"Unknown error" catgets 111 */
	UNKNOWN,				/* 111 - undefined	"Unknown error" catgets 112 */
	UNKNOWN,				/* 112 - undefined	"Unknown error" catgets 113 */
	UNKNOWN,				/* 113 - undefined	"Unknown error" catgets 114 */
	UNKNOWN,				/* 114 - undefined	"Unknown error" catgets 115 */
	UNKNOWN,				/* 115 - undefined	"Unknown error" catgets 116 */
	UNKNOWN,				/* 116 - undefined	"Unknown error" catgets 117 */
	UNKNOWN,				/* 117 - undefined	"Unknown error" catgets 118 */
	UNKNOWN,				/* 118 - undefined	"Unknown error" catgets 119 */
	UNKNOWN,				/* 119 - undefined	"Unknown error" catgets 120 */
	UNKNOWN,				/* 120 - undefined	"Unknown error" catgets 121 */
	UNKNOWN,				/* 121 - undefined	"Unknown error" catgets 122 */
	UNKNOWN,				/* 122 - undefined	"Unknown error" catgets 123 */
	UNKNOWN,				/* 123 - ESOFT	  catgets 124 */
	UNKNOWN,				/* 124 - EMEDIA	  catgets 125 */
	UNKNOWN,				/* 125 - ERELOCATED catgets 126 */
	UNKNOWN,				/* 126 - undefined	"Unknown error" catgets 127 */
	UNKNOWN,				/* 127 - undefined	"Unknown error" catgets 128 */
	UNKNOWN,				/* 128 - undefined	"Unknown error" catgets 129 */
	UNKNOWN,				/* 129 - undefined	"Unknown error" catgets 130 */
	UNKNOWN,				/* 130 - undefined	"Unknown error" catgets 131 */
	UNKNOWN,				/* 131 - undefined	"Unknown error" catgets 132 */
	UNKNOWN,				/* 132 - undefined	"Unknown error" catgets 133 */
	"Pathname is remote",			/* 133 - EPATHREMOTE catgets 134 */
	"Operation completed at server",	/* 134 - EOPCOMPLETE catgets 135 */
	UNKNOWN,				/* 135 - undefined	"Unknown error" catgets 136 */
	UNKNOWN,				/* 136 - undefined	"Unknown error" catgets 137 */
	UNKNOWN,				/* 137 - undefined	"Unknown error" catgets 138 */
	UNKNOWN,				/* 138 - undefined	"Unknown error" catgets 139 */
	UNKNOWN,				/* 139 - undefined	"Unknown error" catgets 140 */
	UNKNOWN,				/* 140 - undefined	"Unknown error" catgets 141 */
	UNKNOWN,				/* 141 - undefined	"Unknown error" catgets 142 */
	UNKNOWN,				/* 142 - undefined	"Unknown error" catgets 143 */
	UNKNOWN,				/* 143 - undefined	"Unknown error" catgets 144 */
	UNKNOWN,				/* 144 - undefined	"Unknown error" catgets 145 */
	UNKNOWN,				/* 145 - undefined	"Unknown error" catgets 146 */
	UNKNOWN,				/* 146 - undefined	"Unknown error" catgets 147 */
	UNKNOWN,				/* 147 - undefined	"Unknown error" catgets 148 */
	UNKNOWN,				/* 148 - undefined	"Unknown error" catgets 149 */
	UNKNOWN,				/* 149 - undefined	"Unknown error" catgets 150 */
	UNKNOWN,				/* 150 - undefined	"Unknown error" catgets 151 */
	UNKNOWN,				/* 151 - undefined	"Unknown error" catgets 152 */
	UNKNOWN,				/* 152 - undefined	"Unknown error" catgets 153 */
	UNKNOWN,				/* 153 - undefined	"Unknown error" catgets 154 */
	UNKNOWN,				/* 154 - undefined	"Unknown error" catgets 155 */
	UNKNOWN,				/* 155 - undefined	"Unknown error" catgets 156 */
	UNKNOWN,				/* 156 - undefined	"Unknown error" catgets 157 */
	UNKNOWN,				/* 157 - undefined	"Unknown error" catgets 158 */
	UNKNOWN,				/* 158 - undefined	"Unknown error" catgets 159 */
	UNKNOWN,				/* 159 - undefined	"Unknown error" catgets 160 */
	UNKNOWN,				/* 160 - undefined	"Unknown error" catgets 161 */
	UNKNOWN,				/* 161 - undefined	"Unknown error" catgets 162 */
	UNKNOWN,				/* 162 - undefined	"Unknown error" catgets 163 */
	UNKNOWN,				/* 163 - undefined	"Unknown error" catgets 164 */
	UNKNOWN,				/* 164 - undefined	"Unknown error" catgets 165 */
	UNKNOWN,				/* 165 - undefined	"Unknown error" catgets 166 */
	UNKNOWN,				/* 166 - undefined	"Unknown error" catgets 167 */
	UNKNOWN,				/* 167 - undefined	"Unknown error" catgets 168 */
	UNKNOWN,				/* 168 - undefined	"Unknown error" catgets 169 */
	UNKNOWN,				/* 169 - undefined	"Unknown error" catgets 170 */
	UNKNOWN,				/* 170 - undefined	"Unknown error" catgets 171 */
	UNKNOWN,				/* 171 - undefined	"Unknown error" catgets 172 */
	UNKNOWN,				/* 172 - undefined	"Unknown error" catgets 173 */
	UNKNOWN,				/* 173 - undefined	"Unknown error" catgets 174 */
	UNKNOWN,				/* 174 - undefined	"Unknown error" catgets 175 */
	UNKNOWN,				/* 175 - undefined	"Unknown error" catgets 176 */
	UNKNOWN,				/* 176 - undefined	"Unknown error" catgets 177 */
	UNKNOWN,				/* 177 - undefined	"Unknown error" catgets 178 */
	UNKNOWN,				/* 178 - undefined	"Unknown error" catgets 179 */
	UNKNOWN,				/* 179 - undefined	"Unknown error" catgets 180 */
	UNKNOWN,				/* 180 - undefined	"Unknown error" catgets 181 */
	UNKNOWN,				/* 181 - undefined	"Unknown error" catgets 182 */
	UNKNOWN,				/* 182 - undefined	"Unknown error" catgets 183 */
	UNKNOWN,				/* 183 - undefined	"Unknown error" catgets 184 */
	UNKNOWN,				/* 184 - undefined	"Unknown error" catgets 185 */
	UNKNOWN,				/* 185 - undefined	"Unknown error" catgets 186 */
	UNKNOWN,				/* 186 - undefined	"Unknown error" catgets 187 */
	UNKNOWN,				/* 187 - undefined	"Unknown error" catgets 188 */
	UNKNOWN,				/* 188 - undefined	"Unknown error" catgets 189 */
	UNKNOWN,				/* 189 - undefined	"Unknown error" catgets 190 */
	UNKNOWN,				/* 190 - undefined	"Unknown error" catgets 191 */
	UNKNOWN,				/* 191 - undefined	"Unknown error" catgets 192 */
	UNKNOWN,				/* 192 - undefined	"Unknown error" catgets 193 */
	UNKNOWN,				/* 193 - undefined	"Unknown error" catgets 194 */
	UNKNOWN,				/* 194 - undefined	"Unknown error" catgets 195 */
	UNKNOWN,				/* 195 - undefined	"Unknown error" catgets 196 */
	UNKNOWN,				/* 196 - undefined	"Unknown error" catgets 197 */
	UNKNOWN,				/* 197 - undefined	"Unknown error" catgets 198 */
	UNKNOWN,				/* 198 - undefined	"Unknown error" catgets 199 */
	UNKNOWN,				/* 199 - undefined	"Unknown error" catgets 200 */
	UNKNOWN,				/* 200 - undefined	"Unknown error" catgets 201 */
	UNKNOWN,				/* 201 - undefined	"Unknown error" catgets 202 */
	UNKNOWN,				/* 202 - undefined	"Unknown error" catgets 203 */
	UNKNOWN,				/* 203 - undefined	"Unknown error" catgets 204 */
	UNKNOWN,				/* 204 - undefined	"Unknown error" catgets 205 */
	UNKNOWN,				/* 205 - undefined	"Unknown error" catgets 206 */
	UNKNOWN,				/* 206 - undefined	"Unknown error" catgets 207 */
	UNKNOWN,				/* 207 - undefined	"Unknown error" catgets 208 */
	UNKNOWN,				/* 208 - undefined	"Unknown error" catgets 209 */
	UNKNOWN,				/* 209 - undefined	"Unknown error" catgets 210 */
	UNKNOWN,				/* 210 - undefined	"Unknown error" catgets 211 */
	UNKNOWN,				/* 211 - undefined	"Unknown error" catgets 212 */
	UNKNOWN,				/* 212 - undefined	"Unknown error" catgets 213 */
	UNKNOWN,				/* 213 - undefined	"Unknown error" catgets 214 */
	UNKNOWN,				/* 214 - undefined	"Unknown error" catgets 215 */

/* dynamic loading errors */
	"Unresolved external",			/* 215 - ENOSYM		catgets 216 */

/* ipc/network software */

/* argument errors */
        "Socket operation on non-socket",	/* 216 - ENOTSOCK 	catgets 217 */
	"Destination address required",		/* 217 - EDESTADDRREQ	catgets 218 */
	"Message too long",			/* 218 - EMSGSIZE	catgets 219 */
	"Protocol wrong type for socket",       /* 219 - EPROTOTYPE	catgets 220 */
	"Protocol not available",		/* 220 - ENOPROTOOPT	catgets 221 */
	"Protocol not supported",		/* 221 - EPROTONOSUPPORT	catgets 222 */
	"Socket type not supported",		/* 222 - ESOCKTNOSUPPORT	catgets 223 */
	"Operation not supported",		/* 223 - EOPNOTSUPP	catgets 224 */
	"Protocol family not supported",	/* 224 - EPFNOSUPPORT	catgets 225 */
	"Address family not supported by protocol family",/*225 - EAFNOSUPPORT	catgets 226 */
	"Address already in use",		/* 226 - EADDRINUSE	catgets 227 */
	"Can't assign requested address",	/* 227 - EADDRNOTAVAIL	catgets 228 */

/* operational errors */
	"Network is down",			/* 228 - ENETDOWN	catgets 229 */
	"Network is unreachable",		/* 229 - ENETUNREACH	catgets 230 */
	"Network dropped connection on reset",	/* 230 - ENETRESET	catgets 231 */
	"Software caused connection abort",	/* 231 - ECONNABORTED	catgets 232 */
	"Connection reset by peer",		/* 232 - ECONNRESET	catgets 233 */
	"No buffer space available",		/* 233 - ENOBUFS	catgets 234 */
	"Socket is already connected",		/* 234 - EISCONN	catgets 235 */
	"Socket is not connected",		/* 235 - ENOTCONN	catgets 236 */
	"Can't send after socket shutdown",	/* 236 - ESHUTDOWN	catgets 237 */
	"Too many references: can't splice",	/* 237 - ETOOMANYREFS	catgets 238 */
	"Connection timed out",			/* 238 - ETIMEDOUT	catgets 239 */
	"Connection refused",			/* 239 - ECONNREFUSED	catgets 240 */
	"Remote peer released connection",	/* 240 - EREMOTERELEASE	catgets 241 */
	"Host is down",				/* 241 - EHOSTDOWN	catgets 242 */
	"No route to host",			/* 242 - EHOSTUNREACH	catgets 243 */
/* Use to be ENET, now obsolete */
	UNKNOWN,				/* 243 - undefined	"Unknown error" catgets 244 */

#if defined (__hp9000s200) || (defined (__hp9000s800) && !defined (comet))
/* non-blocking and interrupt i/o */
	"Operation already in progress",	/* 244 - EALREADY	catgets 245 */
	"Operation now in progress",		/* 245 - EINPROGRESS	catgets 246 */
	"Operation would block",		/* 246 - EWOULDBLOCK	catgets 247 */
#else
	UNKNOWN,                                /* 244 - undefined	"Unknown error" catgets 245 */
	UNKNOWN,                                /* 245 - undefined	"Unknown error" catgets 246 */
	UNKNOWN,                                /* 246 - undefined	"Unknown error" catgets 247 */
#endif

/* Other HPUX errors */
	"Directory not empty",			/* 247 - ENOTEMPTY	catgets 248 */
	"File name too long",			/* 248 - ENAMETOOLONG	catgets 249 */
	"Too many levels of symbolic links",	/* 249 - ELOOP		catgets 250 */

	"No message of desired type",		/* 250 - ENOMSG	  catgets 251 */
	"Function is not available",		/* 251 - ENOSYS	  catgets 252 */
};

#ifdef _NAMESPACE_CLEAN
#   undef sys_nerr
#   pragma _HP_SECONDARY_DEF _sys_nerr sys_nerr
#   define sys_nerr _sys_nerr
#endif
int	sys_nerr = { sizeof(sys_errlist)/sizeof(sys_errlist[0]) -1 };
