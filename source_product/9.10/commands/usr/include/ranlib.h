/* @(#) $Revision: 66.1 $ */       
/*
   ranlib.h:  structure of the "table of contents" for UNIX libraries
*/
#ifndef _RANLIB_INCLUDED /* allows multiple inclusions */
#define _RANLIB_INCLUDED

#ifdef hp9000s500
#define RL_INDEX	"/               "  /* name of table of contents */
#define ORL_INDEX	"__.SYMDEF"	/* old name of table of contents */

struct rl_hdr
	 {
	  long int rl_tcbas;	/* offset of table of contents */
	  long int rl_tclen;	/* size    "   "    "     "    */
	  long int rl_nmbas;	/* offset of name pool */
	  long int rl_nmlen;	/* size    "   "    "   */
	 };
typedef struct rl_hdr RL_HDR;

struct rl_ref
	 {
	  long int name_pos;	/* name pool index for external symbol */
	  long int lib_pos;	/* where to find it in archive file */
	 };
typedef struct rl_ref RL_REF;
#endif /* hp9000s500 */

#ifdef __hp9000s300
/*
 * Structure of the __.SYMDEF table of contents for an archive.
 *
 * ______________________________________________________________________
 * |           								|
 * |	Number of ranlib structures in __.SYMDEF (1 word)(rantnum in ld)|
 * |____________________________________________________________________|
 * |									|
 * |	Sizeof string table (# bytes) - 1 long (rasciisize in ld)	|
 * |____________________________________________________________________|
 * |									|
 * |	String table (asciz strings)					|
 * |____________________________________________________________________|
 * |									|
 * |	ranlib structure array (1 per defined external sym)		|
 * |____________________________________________________________________|
 */

#define DIRNAME "/               "
#define ODIRNAME "__.SYMDEF"

#ifndef _OFF_T
#  define _OFF_T
   typedef long off_t;
#endif /* _OFF_T */

struct	ranlib {
	union {
		off_t	ran_strx;	/* string table index of this sym */
		char	*ran_name;
	} ran_un;
	off_t	ran_off;		/* library member is at this offset */
};
#endif /* __hp9000s300 */

#endif /* _RANLIB_INCLUDED */
