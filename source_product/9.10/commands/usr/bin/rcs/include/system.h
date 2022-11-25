/* $Header: system.h,v 66.1 89/09/28 15:47:28 smp Exp $ */

/*
 * System-dependent definitions
 *
 * Author: Bob Boothby
 */

#ifdef hpux /* any HP "SYSTEM V" implementation? */
#define HP_UX
#else
#ifdef hp9000ipc
#define HP_UX
#else
#ifdef hp9000s200
#define HP_UX
#else
#ifdef hp9000s500
#define HP_UX
#endif /* hp9000s500 */
#endif /* hp9000s200 */
#endif /* hp9000ipc */
#endif /* any HP "SYSTEM V" implementation? */

/* DATEFORM must define one of; %02d or %.2d to prints numbers with a
 * field width 2, with leading zeroes. For example, 0, 1, and 22 must
 * be printed as 00, 01, and * 22. Otherwise, there will be problems
 * with the dates.
 */
#ifdef HP_UX
#define rindex strrchr
#define DATEFORM  "%.2d.%.2d.%.2d.%.2d.%.2d.%.2d"
#ifdef LONGFILENAMES
#define NAME_MAX 255
#else
#define NAME_MAX 14
#endif LONGFILENAMES
#define RENAME mvfile
#define TIMEZONE extern timezone;
/* local time zone for maketime */
#define LOCALZONE return(_lclzon >= 0 ? _lclzon : (_lclzon = timezone/60));
/* how to send mail */
#define SENDMAIL sprintf(command,"/usr/bin/mailx %s < %s",who,messagefile);
#else
#define DATEFORM  "%02d.%02d.%02d.%02d.%02d.%02d"
#define NAME_MAX 255
#define RENAME rename
/* local time zone for maketime */
#define TIMEZONE
#define LOCALZONE struct timeb tb;if(_lclzon<0){ftime(&tb);_lclzon=tb.timezone;}return(_lclzon);
/* how to send mail */
#define SENDMAIL sprintf(command,"/usr/lib/sendmail %s < %s",who,messagefile);
#endif

#define EX_OK 0			/* No Error return from "system() */
#define NCPFN	255    		/* number of characters per filename	*/
       /* make 255 since we need the string space */
#define BYTESIZ	8		/* number of bits in a byte		*/
