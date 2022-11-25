/* config.h
 * This file was produced by running the config.h.SH script, which
 * gets its values from config.sh, which is generally produced by
 * running Configure.
 *
 * Feel free to modify any of this as the need arises.  Note, however,
 * that running config.h.SH again will wipe out any changes you've made.
 * For a more permanent change edit config.sh and rerun config.h.SH.
 */


/* EUNICE:
 *	This symbol, if defined, indicates that the program is being compiled
 *	under the EUNICE package under VMS.  The program will need to handle
 *	things like files that don't go away the first time you unlink them,
 *	due to version numbering.  It will also need to compensate for lack
 *	of a respectable link() command.
 */
/* VMS:
 *	This symbol, if defined, indicates that the program is running under
 *	VMS.  It is currently only set in conjunction with the EUNICE symbol.
 */
/*#undef	EUNICE		/**/
/*#undef	VMS		/**/

/* CPPSTDIN:
 *	This symbol contains the first part of the string which will invoke
 *	the C preprocessor on the standard input and produce to standard
 *	output.	 Typical value of "cc -E" or "/lib/cpp".
 */
/* CPPMINUS:
 *	This symbol contains the second part of the string which will invoke
 *	the C preprocessor on the standard input and produce to standard
 *	output.  This symbol will have the value "-" if CPPSTDIN needs a minus
 *	to specify standard input, otherwise the value is "".
 */
#define CPPSTDIN "/lib/cpp"
#define CPPMINUS ""

/* FCNTL:
 *	This symbol, if defined, indicates to the C program that it should
 *	include fcntl.h.
 */
#define	FCNTL		/**/

/* GETOPT:
 *	This symbol, if defined, indicates that the getopt() routine exists.
 */
#define	GETOPT		/**/

/* IOCTL:
 *	This symbol, if defined, indicates that sys/ioctl.h exists and should
 *	be included.
 */
#define	IOCTL		/**/

/* MEMCPY:
 *	This symbol, if defined, indicates that the memcpy routine is available
 *	to copy blocks of memory.  Otherwise you should probably use bcopy().
 *	If neither is defined, roll your own.
 */
#define	MEMCPY		/**/

/* MKDIR:
 *	This symbol, if defined, indicates that the mkdir routine is available
 *	to create directories.  Otherwise you should fork off a new process to
 *	exec /bin/mkdir.
 */
#define	MKDIR		/**/

/* STRCSPN:
 *	This symbol, if defined, indicates that the strcspn routine is available
 *	to scan strings.
 */
#define	STRCSPN		/**/

/* TMINSYS:
 *	This symbol is defined if this system declares "struct tm" in
 *	in <sys/time.h> rather than <time.h>.  We can't just say
 *	-I/usr/include/sys because some systems have both time files, and
 *	the -I trick gets the wrong one.
 */
#define	TMINSYS 	/**/

/* vfork:
 *	This symbol, if defined, remaps the vfork routine to fork if the
 *	vfork() routine isn't supported here.
 */
/*#undef	vfork fork	/**/

/* VOIDSIG:
 *	This symbol is defined if this system declares "void (*signal())()" in
 *	signal.h.  The old way was to declare it as "int (*signal())()".  It
 *	is up to the package author to declare things correctly based on the
 *	symbol.
 */
#define	VOIDSIG 	/**/

/* GIDTYPE:
 *	This symbol has a value like gid_t, int, ushort, or whatever type is
 *	used to declare group ids in the kernel.
 */
#define GIDTYPE gid_t		/**/

/* Reg1:
 *	This symbol, along with Reg2, Reg3, etc. is either the word "register"
 *	or null, depending on whether the C compiler pays attention to this
 *	many register declarations.  The intent is that you don't have to
 *	order your register declarations in the order of importance, so you
 *	can freely declare register variables in sub-blocks of code and as
 *	function parameters.  Do not use Reg<n> more than once per routine.
 */

#define Reg1 register		/**/
#define Reg2 register		/**/
#define Reg3 register		/**/
#define Reg4 register		/**/
#define Reg5 register		/**/
#define Reg6 register		/**/
#define Reg7 		/**/
#define Reg8 		/**/
#define Reg9 		/**/
#define Reg10 		/**/
#define Reg11 		/**/
#define Reg12 		/**/
#define Reg13 		/**/
#define Reg14 		/**/
#define Reg15 		/**/
#define Reg16 		/**/

/* UIDTYPE:
 *	This symbol has a value like uid_t, int, ushort, or whatever type is
 *	used to declare user ids in the kernel.
 */
#define UIDTYPE uid_t		/**/

#include		<sys/types.h>	/**/
#include		<dirent.h>	/**/
#ifdef hp9000s300
/*#undef	 direct	/* brain-dead HP headers */
#else /* hp9000s300 */
/*#undef	direct		/* normal Berkeleyisms */
#endif /* hp9000s300 */
/* index:
 *	This preprocessor symbol is defined, along with rindex, if the system
 *	uses the strchr and strrchr routines instead.
 */
/* rindex:
 *	This preprocessor symbol is defined, along with index, if the system
 *	uses the strchr and strrchr routines instead.
 */
#define	index strchr	/* cultural */
#define	rindex strrchr	/*  differences? */

/* LIMITSH:
 *	This symbol, if defined, indicates that the <limits.h> header is 
 *	available and should be used to define system limits.
 */
#define	LIMITSH		/**/

#include		<sys/sysmacros.h>	/* major,minor,mkdev macros */

/* MEMSET:
 *	This symbol, if defined, indicates that the memset routine is available
 *	to copy blocks of memory.  Otherwise you should probably use bcopy().
 *	If neither is defined, roll your own.
 */
#define	MEMSET		/**/

/* RMDIR:
 *	This symbol, if defined, indicates that the rmdir routine is 
 *	available to scan strings.
 */
#define	RMDIR		/**/

/* STRERROR:
 *	This symbol, if defined, indicates that the strerror routine is 
 *	available to print error messages.
 */
#define	STRERROR		/**/

/* STRTOK:
 *	This symbol, if defined, indicates that the strtok routine is 
 *	available to scan strings.
 */
#define	STRTOK		/**/

/* OFFSET:
 *	This symbol has a value like long, off_t, or whatever type is
 *	used to declare file offsets in the kernel.
 */
#define OFFSET off_t		/**/

