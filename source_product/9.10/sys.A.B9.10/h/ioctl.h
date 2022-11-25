/* $Header: ioctl.h,v 1.42.83.5 93/12/08 18:23:49 marshall Exp $ */     

#ifndef _SYS_IOCTL_INCLUDED
#define _SYS_IOCTL_INCLUDED

/*
 * ioctl() definitions
 */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

/* Function prototype for ioctl() */

#ifndef _KERNEL
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#ifdef _PROTOTYPES
     extern int ioctl(int, int, ...);
#else /* not _PROTOTYPES */
     extern int ioctl();
#endif /* not _PROTOTYPES */

#ifdef __cplusplus
   }
#endif /* __cplusplus */
#endif /* not _KERNEL */



#ifndef _IO
/*
 * Ioctl's have the command encoded in the lower 16 bits,
 * and the size of any in or out parameters in the upper
 * 16 bits.  The high 2 (or 3 on s800) bits of the upper 16 bits are used
 * to encode the in/out status of the parameter.
 */
#ifdef __hp9000s300
#define IOCSIZE_MASK    0x3fff0000      /* Field which has parameter size */
#endif /* __hp9000s300 */
#ifdef __hp9000s800
#define IOCSIZE_MASK    0x1fff0000      /* Field which has parameter size */
#endif /* __hp9000s800 */

#define IOCPARM_MASK    (IOCSIZE_MASK>>16) /* maximum parameter size */
                                           /* parameters must be < 16K bytes
                                            * on s300 and < 8K bytes on s800.
                                            */

/* The following two defines are for backward compatibility with code
 * expecting a 128-byte maximum ioctl parameter size.
 */
#define EFFECTIVE_IOCSIZE_MASK  0x7f0000
#define EFFECTIVE_IOCPARM_MASK  (EFFECTIVE_IOCSIZE_MASK>>16)

#ifdef __hp9000s300
#define FIONREAD        _IOR('f', 127, int)     /* get # bytes to read */
#endif /* __hp9000s300 */
#ifdef __hp9000s300
#define FIONBIO         _IOW('f', 126, int)     /* set/clear non-blocking i/o */
#define FIOSNBIO        FIONBIO
#define FIOASYNC        _IOW('f', 125, int)     /* set/clear async i/o */
#define FIOSSAIOSTAT    _IOW('f', 122, int)     /* set/clear async i/o
                                                   signaling */
                                                /* if arg == 0, clear */
                                                /* otherwise, set */
#define FIOGSAIOSTAT     _IOR('f', 121, int)    /* arg is set: */
                                                /*   to 1, if process will be
                                                     signaled */
                                                /*   to 0, otherwise */
#define FIOSSAIOOWN     _IOW('f', 120, int)     /* set sys async i/o owner */
#define FIOGSAIOOWN     _IOR('f', 119, int)     /* get sys async i/o owner */
#define FIOGNBIO        _IOR('f', 117, int)     /* arg is set: */
                                                /*   to 1, if non-blocking */
                                                /*   to 0, otherwise */
#endif  /* __hp9000s300 */
#define IOCCMD_MASK     0x0000ffff      /* command field */

#ifdef __hp9000s300
#define IOC_VOID        0x80000000      /* no parameters, field size = 0 */
#endif /* __hp9000s300 */
#ifdef __hp9000s800
#define IOC_VOID        0x20000000      /* no parameters */
/* the 0x20000000 is so we can distinguish new ioctl's from old */
#endif /* __hp9000s800 */

#define IOC_OUT         0x40000000      /* copy out parameters */
#define IOC_IN          0x80000000      /* copy in parameters */
#define IOC_INOUT       (IOC_IN|IOC_OUT)
#define _IO(x,y)        (IOC_VOID|((x)<<8)|y)
#define _IOR(x,y,t)     (IOC_OUT|((sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|y)
#define _IOW(x,y,t)     (IOC_IN|((sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|y)
/* this should be _IORW, but stdio got there first */
#define _IOWR(x,y,t)    (IOC_INOUT|((sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|y)

#ifdef BELL5_2  /*Not really Bell's, just for place holding*/
/* The following ioctl commands are not currently supported by HPUX */
/* another local */
#define FIONREAD        _IOR('f', 127, int)     /* get # bytes to read */
#define FIONBIO         _IOW('f', 126, int)     /* set/clear non-blocking i/o */
#define FIOASYNC        _IOW('f', 125, int)     /* set/clear async i/o */
#define FIOSETOWN       _IOW('f', 124, int)     /* set owner */
#define FIOGETOWN       _IOR('f', 123, int)     /* get owner */
#endif /* BELL5_2 */

#endif /* _IO */

#ifdef __hp9000s800
/*
 * tty ioctl commands
 */

#ifdef  notdef
#define TIOCNOTTY       _IO('t', 113)           /* void tty association */
#endif /* notdef */

#define         TIOCPKT_DATA            0x00    /* data packet */
#define         TIOCPKT_FLUSHREAD       0x01    /* flush packet */
#define         TIOCPKT_FLUSHWRITE      0x02    /* flush packet */
#define         TIOCPKT_STOP            0x04    /* stop output */
#define         TIOCPKT_START           0x08    /* start output */
#define         TIOCPKT_NOSTOP          0x10    /* no more ^S, ^Q */
#define         TIOCPKT_DOSTOP          0x20    /* now do ^S ^Q */

#define FIONREAD        _IOR('f', 127, int)     /* get # bytes to read */
#define FIONBIO         _IOW('f', 126, int)     /* set/clear non-blocking i/o */
#define FIOASYNC        _IOW('f', 125, int)     /* set/clear async i/o */

/*
 * ioctl commands used by ktestio so leave in for now 
 */

#define FIOSETASYNC     _IOW('f', 124, int)     /* set/clear async I/O
                                                   signaling */
#define FIOGETASYNC     _IOR('f', 123, int)     /* arg is set: */
                                                /*   to 1, if process will be
                                                     signaled */
                                                /*   to 0, if process will not
                                                     be signaled */
#define FIONBRW         _IOW('f', 122, int)     /* perform read, partial write
                                                   non-blocking I/O */
#define FIONOBLK        _IOW('f', 122, int)     /* perform partial write
                                                   non-blocking I/O */
/*
 * end of ioctl commands used by ktestio so leave in for now
 */
#define FIOSSAIOSTAT    _IOW('f', 122, int)     /* set/clear async i/o
                                                   signaling */
                                                /* if arg == 0, clear */
                                                /* otherwise, set */
#define FIOGSAIOSTAT    _IOR('f', 121, int)     /* arg is set: */
                                                /*   to 1, if process will be
                                                     signaled */
                                                /*   to 0, otherwise */
#define FIOSSAIOOWN     _IOW('f', 120, int)     /* set sys async i/o owner */
#define FIOGSAIOOWN     _IOR('f', 119, int)     /* get sys async i/o owner */
#define FIOSNBIO        _IOW('f', 118, int)     /* set/clear non-blocking i/o */
                                                /* if arg == 0, clear */
                                                /* otherwise, set */
#define FIOGNBIO        _IOR('f', 117, int)     /* arg is set: */
                                                /*   to 1, if non-blocking */
                                                /*   to 0, otherwise */
#endif /* __hp9000s800 */

#ifdef __hp9000s800
#define FIONBIO         _IOW('f', 126, int)     /* set/clear non-blocking i/o */
#define FIOASYNC        _IOW('f', 125, int)     /* set/clear async i/o */
#endif /* __hp9000s800 */

/* socket i/o controls */
#define SIOCSHIWAT      _IOW('s',  0, int)              /* set high watermark */
#define SIOCGHIWAT      _IOR('s',  1, int)              /* get high watermark */
#define SIOCSLOWAT      _IOW('s',  2, int)              /* set low watermark */
#define SIOCGLOWAT      _IOR('s',  3, int)              /* get low watermark */
#define SIOCATMARK      _IOR('s',  7, int)              /* at oob mark? */
#define SIOCSPGRP       _IOW('s',  8, int)              /* set process group */
#define SIOCGPGRP       _IOR('s',  9, int)              /* get process group */

#define SIOCADDRT       _IOW('r', 10, struct rtentry)   /* add route */
#define SIOCDELRT       _IOW('r', 11, struct rtentry)   /* delete route */
#define SIOCGTRTADDR    _IOR('r', 12, struct routeaddrs)/* rtn rte strct ptrs*/

#define SIOCSIFADDR     _IOW('i', 12, struct ifreq)     /* set ifnet address */
#define SIOCGIFADDR     _IOWR('i',13, struct ifreq)     /* get ifnet address */
#define SIOCSIFDSTADDR  _IOW('i', 14, struct ifreq)     /* set p-p address */
#define SIOCGIFDSTADDR  _IOWR('i',15, struct ifreq)     /* get p-p address */
#define SIOCSIFFLAGS    _IOW('i', 16, struct ifreq)     /* set ifnet flags */
#define SIOCGIFFLAGS    _IOWR('i',17, struct ifreq)     /* get ifnet flags */
#define SIOCGIFBRDADDR  _IOWR('i',18, struct ifreq)     /* get broadcast addr */
#define SIOCSIFBRDADDR  _IOW('i',19, struct ifreq)      /* set broadcast addr */
#define SIOCGIFCONF     _IOWR('i',20, struct ifconf)    /* get ifnet list */
#define SIOCGIFNETMASK  _IOWR('i',21, struct ifreq)     /* get net addr mask */
#define SIOCSIFNETMASK  _IOW('i',22, struct ifreq)      /* set net addr mask */
#define SIOCGIFMETRIC   _IOWR('i',23, struct ifreq)     /* get IF metric */
#define SIOCSIFMETRIC   _IOW('i',24, struct ifreq)      /* set IF metric */

#define SIOCSARP        _IOW('i', 30, struct arpreq)    /* set arp entry */
#define SIOCGARP        _IOWR('i',31, struct arpreq)    /* get arp entry */
#define SIOCDARP        _IOW('i', 32, struct arpreq)    /* delete arp entry */

#define SIOCADDMULTI    _IOW('i', 49, struct ifreq)     /* add m'cast addr */
#define SIOCDELMULTI    _IOW('i', 50, struct ifreq)     /* del m'cast addr */

#define SIOCGNIT        _IOWR('p',52, struct nit_ioc)   /* get NIT control */
#define SIOCSNIT        _IOW('p',53, struct nit_ioc)    /* set NIT control */

#define SIOCTARP        _IOW('i', 244, struct arpreq)   /* test arp */
#define SIOCPROXYON     _IOW('s', 245, struct prxentry) /* enable proxy server*/
#define SIOCPROXYOFF    _IOW('s', 246, struct prxentry) /* disable  "     "   */
#define SIOCPROXYADD    _IOW('s', 247, struct prxentry) /* add proxy entry */
#define SIOCPROXYDELETE _IOW('s', 248, struct prxentry) /* delete proxy entry */
#define SIOCPROXYSHOW   _IOW('s', 249, struct prxentry) /* show proxy entry */
#define SIOCPROXYLIST   _IOW('s', 250, struct prxentry) /* show all entries */
#define SIOCPROXYFLUSH  _IOW('s', 251, struct prxentry) /* flush proxy table */
#define SIOCPROXYAPPEND _IOW('s', 252, struct prxentry) /* add to proxy entry */
#define SIOCSACFLAGS    _IOW('i', 253, struct ifreq)    /* set arpcom flags */
#define SIOCGACFLAGS    _IOWR('i', 254, struct ifreq)   /* get arpcom flags */
#define SIOCJNVS        _IOW('s', 255, int)        /* join NVS pty and socket */

#define NMIOGET         _IOWR('m', 1, struct nmparms)   /* get net mgmt obj */
#define NMIOSET         _IOW('m', 2, struct nmparms)    /* set net mgmt obj */
#define NMIODEL         _IOW('m', 3, struct nmparms)    /* set net mgmt obj */
#define NMIOCRE         _IOW('m', 4, struct nmparms)    /* set net mgmt obj */

#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef _UNSUPPORTED

        /* 
         * NOTE: The following header file contains information specific
         * to the internals of the HP-UX implementation. The contents of 
         * this header file are subject to change without notice. Such
         * changes may affect source code, object code, or binary
         * compatibility between releases of HP-UX. Code which uses 
         * the symbols contained within this header file is inherently
         * non-portable (even between HP-UX implementations).
        */
#ifdef _KERNEL_BUILD
#       include "../h/_ioctl.h"
#else  /* ! _KERNEL_BUILD */
#       include <.unsupp/sys/_ioctl.h>
#endif /* _KERNEL_BUILD */
#endif /* _UNSUPPORTED */

#endif /* _SYS_IOCTL_INCLUDED */
