/* @(#) $Revision: 72.2 $ */
/*
***************************************************************************
** This is a set of library routines for code set conversion.
** The X/OPEN command iconv(1) is implemented in terms of these routines
**
** Here are the entry points:
** 
**	int iconvsize (tocode, fromcode)
**	char *tocode;
**	char *fromcode;
** 
**	iconvd iconvopen (tocode, fromcode, table, d1, d2)
**	char *tocode;
**	char *fromcode;
**	char *table;
**	int d1;
**	int d2;
** 
**	int iconvclose (cd)
**	iconvd cd;
**
**	int iconvlock( cd, direction, lock, s)
**	iconvd cd;
**	int direction;
**	int lock;
**	char *s;
** 
**	int ICONV (cd, inchar, inbytesleft, outchar, outbytesleft)
**	iconvd cd;
**	unsigned char **inchar;
**	int *inbytesleft;
**	unsigned char **outchar;
**	int *outbytesleft;
** 
**	int ICONV1 (cd, to, from, buflen)
**	iconvd cd;
**	char *to;
**	char *from;
**	int buflen;
** 
**	int ICONV2 (cd, to, from, buflen)
**	iconvd cd;
**	char *to;
**	char *from;
**	int buflen;
** 
** The general idea is that iconvsize and iconvinit are called
** once initially to set up the conversion tables and routines.
** Then ICONV is called (possibly within a loop) to get a
** character in the "converted from" code set from an input
** buffer, convert the character to the "converted to" code set
** and place it plus any lock-shift information into an output
** buffer.  ICONV, ICONV1, ICONV2, are macros that expand to
** pointers to functions (__iconv, __iconv1, __iconv2) that actually
** do the conversion.  These macros are defined in iconv.h.
** 
** A conversion descriptor is used to identify the conversion. 
** This was put in primarily for the IND emulators which need several
** conversions open at the same time (HP to IBM, IBM to HP).
** 
** Iconvsize gives the user the size of the conversion table
** and the user allocates and de-allocates the table.
** The user is free to place the table where he wants (user space,
** shared memory), then pass a pointer to iconvopen which reads in the table.
** This flexibility could be useful for multi-byte codesets which have large
** tables.
** 
** ICONV uses a pointer and a count for both input and output buffers.
** This runs counter to the strcpy(3c) model using null terminated strings.
** A pointer and a count is used here because it might not be so easy on some
** data files to detect the end-of-line.  The character conversion routines
** might be used on non-Unix files.  Also a single input character may map
** onto several output characters.  For example, when going from Japanese15
** to JIS you need to put in lock-shift escape sequence to delimit single- 
** and multi-byte characters.  Since there is no way to know how big to make
** the output buffer, it might be better to work with a fixed length buffer
** and have the routine to check for overflow conditions.
** 
** ICONV1 and ICONV2 handle single- and multi-byte characters separately.
** The IND terminal emulators recognize several terminal protocols to
** identify single- and multi-byte characters.  ICONV1 and ICONV2 are used
** so that the protocol recognition code used by the emulators does not have
** to be duplicated in the conversion routine.
**
** A note on default characters:
**
** 1. Single-byte code sets
**
** For single-byte code sets, it is assumed that the translation table
** forces a 1-to-1 mapping between the "from" and "to" characters.
** This 1-to-1 mapping guarantees that the conversion is reversible.
** 
** 	iconv -f roman8 -t iso8859_1 file1 > output
** 	iconv -f iso8859_1 -t roman8 output > file2
** 	diff file1 file2
** 
** "file1" and "file2" are the same since there is a (forced) 1-to-1
** correspondence between the two code sets.  No default characters
** are used with single-byte code sets.
**
** 2. Multi-byte code sets
** 
** For multi-byte code sets, unmapped character are flagged in the table 
** using the 8-bit DEL character (0xff).  The "from" character, in this
** case, is then mapped to the default character passed into iconvopen.
** This approach was taken since different multi-byte code sets usually 
** don't have the same number of characters which makes a 1-to-1 mapping
** difficult.  Also unused sections in multi-byte code sets are usually
** reserved for future use.
**
** Here are the steps to add a new conversion routine:
**	1. If a direct translation table is used:
**		a. add the table to /usr/lib/iconv/direct
**		b. define the table strucure and add it to the TBLTYPE union.
**		c. add a new code set class flag to the table based section
**		   (DIRECT8, HP15 ...) if the current ones do not apply.
**		d. add the code to read the table in the read_direct() routine.
**		e. code the translation routines
**		f. add the name of the translation routines to the table part
**		   of the convert lookup table in iconvopen()
**	2. If an algorithm is used:
**		a. add only a table header to /usr/lib/iconv/direct
**		b. add a new code set class flag to the algorithmic based
**		   section (JIS, UJIS ...) if the current ones do not apply.
**		c. code the translation routines
**		d. add the name of the translation routines to the algorithm
**		   part of the convert lookup table in iconvopen()
***************************************************************************
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define iconv _iconv
#define iconvlock _iconvlock
#define iconvclose _iconvclose
#define iconv_close _iconv_close
#define iconvopen _iconvopen
#define iconv_open _iconv_open
#define iconvsize _iconvsize
#define strtol _strtol
#define strncmp _strncmp
#define strlen _strlen
#define strcpy _strcpy
#define strncpy _strncpy
#define strcat _strcat
#define strncat _strncat
#define open _open
#define read _read
#define lseek _lseek
#define close _close
#define bsearch _bsearch
#define stat _stat
#define qsort _qsort
#endif

/*
***************************************************************************
*** include files 
***************************************************************************
*/

#include <stdio.h>		/* for NULL */
#include <fcntl.h>		/* for O_RDONLY */
#include <sys/types.h>		/* for fstat */
#include <sys/stat.h>		/* for fstat */
#include <search.h>		/* for bsearch */
#include <ctype.h>		/* for isspace */
#include <string.h>		/* for strcpy, strcat ... */
#include <setlocale.h>		/* for locale table */
#include <limits.h>		/* for OPEN_MAX definition */
#include <errno.h>		/* for errnos */
#include <iconv.h>
#ifdef _THREAD_SAFE
#include "rec_mutex.h"

extern REC_MUTEX _iconv_rmutex;
#endif

#ifndef OPEN_MAX		/* temporary definition */
#define	OPEN_MAX	60
#endif
/*
***************************************************************************
*** some constants
***************************************************************************
*/

#define	GOOD			0		/* successful return value */
#define	BAD			-1		/* unsuccessful return value */
#define	TRUE			1		/* boolean true */
#define	FALSE			0		/* boolean false */
#define	DEL			0xff		/* 8-bit del character */
#define	BAD_CHAR		DEL		/* bad character flag */
#define	BAD_SEQ			0xffff		/* bad sequence number */
#define MAX_EUC_LEN		10		/* for EUC set char length */
#define MAX_LOCK		16		/* max lock shift string len */
#define CNAME_LEN		4		/* code set name len */
#define	FROM			0		/* init "from" flag */
#define	TO			1		/* init "to" flag */
#define K			1024		/* 1 K */
#define _64K			64*K		/* 64 K */

/* return values of ICONV */

#define IN_CHAR_TRUNC	1
#define BAD_IN_CHAR	2
#define OUT_CHAR_TRUNC	3

/* size of conversion code table */

#define SIZE_CCODES 		256
#define SIZE_8DIRECT 		256

/* characters */

#define SIN			0x0f		/* shift-in */
#define SOUT			0x0e		/* shift-out */
#define ESC			0x1b		/* escape */
#define SS2			0x8e		/* single shift-2 */
#define SS3			0x8f		/* single shift-3 */

/* states */

#define START			0		/* for jis and ebcdic */
#define SINGLE_BYTE		1		/* for ebcdic */
#define DOUBLE_BYTE		2		/* for ebcdic */
#define KANJI			3		/* for jis */
#define KANJI2			4		/* for jis III */

/* header constants */

#define LAST_VERSION		0		/* last version number:
						   table & code must agree */
#define HEADER_SIZE		64		/* for table header */
#define MAX_DEFAULT		2		/* for default character */
#define MAX_NAME		256		/* for language names */

/* header file code set class flags,
   if new class added must change LAST_CLASS */

/* table based routines */
#define DIRECT8			0
#define CCODES8			1
#define HP15			2
#define EBCDIC16		3
#define TJIS			4
#define EUC			5

/* algorithmic based routines */
#define JIS			6
#define UJIS			7
#define SJIS			8

#define FIRST_CLASS		DIRECT8
#define LAST_CLASS		SJIS
#define ALGO			JIS

/* number of conversion descriptors */
#define MAX_CD			10

/* mask hi-bit */

#define HIBITSET( i)		((i) & 0x80)
#define HIBITON( i)		((i) | 0x80)
#define HIBITOFF( i)		((i) & 0x7f)

/*
***************************************************************************
*** typedefs 
***************************************************************************
*/

typedef struct {
	int len;			/* length of character string */
	char str[MAX_LOCK];		/* ptr to null terminated char string */
} iconvele;

typedef struct {

	/* state Information */

	int state;			/* state variable */

	/* lock-shift functions */

	iconvele LS0;
	iconvele LS1;
	iconvele LS2;

	/* EUC set lengths for sets 0, 1, 2 and 3 */
	int setlen[4];

} iconvinfo;


typedef unsigned char UCHAR;

typedef unsigned short int UINT;

typedef int (*PFI) ();		/* ptr to function returning int type */

typedef UCHAR CH2[2];		/* two-byte character type */

typedef struct {
	UINT *fof2;		/* 1st-of-2 offset table */
	UINT *sof2;		/* 2nd-of-2 offset table */
	UCHAR *one;		/* one-byte translation table */
	CH2 **two;		/* two-byte translation table */
} ONETWOTYPE;

typedef union {
	ONETWOTYPE _1_2;	/* structure for 1 & 2 byte code sets */
} TBLTYPE;

typedef struct {
	UINT conv_code;		/* intermediate conversion code */
	UCHAR char_code;	/* code set character */
} CONVCODE;

typedef struct {
	char *name;		/* ptr to code set name */
	int index;		/* "from" or "to" routines index */
} NOTABLE;

/*
**************************************************************************
*** Default Lock-Shift Information
**************************************************************************
*/

#define SI		"\017"
#define SO		"\016"
#define KO		"\033(J"
#define KI		"\033$B"
#define KI2		"\033$C"

/*
***************************************************************************
*** locale table defines
***************************************************************************
*/

#define HDR_SIZE	sizeof( struct table_header) 
#define TBL_SIZE	256
#define NLSDIR		"/usr/lib/nls/"
#define TBL_NAME	"/locale.inf"

/*
***************************************************************************
*** iconv table directory
***************************************************************************
*/

#define DEFAULT_DIR	"/usr/lib/iconv"
#ifdef _THREAD_SAFE
/*
 * There are a considerable number of static variables used in this
 * module that are not stored on a per conversion descriptor basis.
 * However, none of the variables need to have their values
 * maintained across iconv calls, except for Toindex and Fromindex.
 * The code in iconvopen() has been modified to take care of the
 * latter case.
 */
#endif


char *_Dir = DEFAULT_DIR;		/* iconv table directory */
static char Fromname[MAX_NAME];		/* pathname of "converted from" table */
static char Toname[MAX_NAME];		/* pathname of "converted to" table */

/*
***************************************************************************
*** Table header information
***************************************************************************
*/

static char Fromlangname[MAX_NAME];	/* language of "from" codeset */
static char Tolangname[MAX_NAME];	/* language of "to" codeset */
static int Fromindex;			/* flag of "from" code set */
static int Toindex;			/* flag of "to" code set */
static UCHAR Fof2_max;			/* max num 1st-of-2 in xlate table */
static UCHAR Sof2_max;			/* max num 2nd-of-2 in xlate table */
static int Euc_setlen[3];		/* length of EUC Set 1, 2, 3 */

/*
***************************************************************************
*** Save input character variables
***************************************************************************
*/

static UCHAR *From;			/* ptr to input char */
static int Frombytes;			/* num bytes left in input buffer */

/*
***************************************************************************
*** Information Associated with a Conversion Descriptor
***************************************************************************
*/
PFI __iconv[OPEN_MAX];			/* character conversion routine */
PFI __iconv1[MAX_CD];			/* character conversion routine */
PFI __iconv2[MAX_CD];			/* character conversion routine */
static iconvinfo cs_in[OPEN_MAX];	/* information about "from" code set */
static iconvinfo cs_out[OPEN_MAX];	/* information about "to" code set */
static int Conv_descriptor[OPEN_MAX];	/* conversion descritor array */
static UCHAR Default1[MAX_CD];		/* single-byte default character */
static UCHAR Default2[MAX_CD][MAX_DEFAULT];	/* multi-byte default char */
static UCHAR Firstof2[OPEN_MAX][256];	/* 1st-of-2 table */
static UCHAR Secof2[OPEN_MAX][256];	/* 2nd-of-2 table */
static TBLTYPE Table[OPEN_MAX];		/* translation table */
static UCHAR XPG_flag [OPEN_MAX];	/* flag indicating that XPG4 routines
					 * are used */
static UCHAR *tab_alloc[OPEN_MAX];	/* save allocated table pointers */
static UCHAR XPG_default[MAX_DEFAULT] =	{ /* XPG4 single/multi byte def. char */
				0xff,
				0xff
				};
/*
**************************************************************************
*** Space for JIS, SJIS and EUC characters
**************************************************************************
*/

static UCHAR Ss1[MAX_EUC_LEN];			/* EUC set 1 char */
static UCHAR Ss2[MAX_EUC_LEN] = { SS2 };	/* EUC set 2 char */
static UCHAR Ss3[MAX_EUC_LEN] = { SS3 };	/* EUC set 3 char */

static UCHAR Sjis[2];
static UCHAR *Jis = Sjis;

#ifdef _THREAD_SAFE
#define RETURN  _rec_mutex_unlock(&_iconv_rmutex); return
#else
#define RETURN  return
#endif

/*
***************************************************************************
** Get input character macro: Save start of the current input character,
** and point to the next input character.
**	len : length of input character
***************************************************************************
*/

#define MOVEIN( len)							\
	if (*inbytesleft < (len)) {	/* room for char ? */		\
		if (*xpgflg) {						\
			errno = EINVAL;					\
			RETURN BAD;					\
		} else							\
			RETURN IN_CHAR_TRUNC;				\
	}								\
	From = *inchar;			/* save current char */		\
	Frombytes = *inbytesleft;	/* save current input bytes */	\
	*inchar += (len);		/* point to next char */	\
	*inbytesleft -= (len);		/* new input bytes left */

/*
***************************************************************************
** Put output character macro: Place the current output character in output
** buffer, and find to the next slot in the output buffer.  If there isn't
** room for the output character, point to the input character that caused 
** the overflow and leave.
**	i : loop counter
**	pxlch : pointer to the translated character
**	len : length of output character
***************************************************************************
*/

#define MOVEOUT( i, pxlch, len)						\
	/* room for xlated char ? */					\
	if (*outbytesleft < (len)) {					\
		/* restore input buffer variables */			\
		*inchar = From;						\
		*inbytesleft = Frombytes;				\
		if(*xpgflg) {						\
			errno = E2BIG;					\
			RETURN BAD;					\
		} else							\
			RETURN OUT_CHAR_TRUNC;				\
	}								\
	/* move char to ouput buffer & decrement output bytes left */	\
	for (i=0 ; i < (len) ; i++) {					\
		*(*outchar)++ = *((pxlch)+i);				\
		(*outbytesleft)--;					\
	}

/*
***************************************************************************
** Translate single-byte character: translated character obtained by
** indexing into the table with the input character.  If there is no
** corresponding translated character for the input character, use the
** default character.
**	pxlch : pointer to the translated character
***************************************************************************
*/

#define XLT_1( pxlch)							\
	if (*(pxlch = &Table[cd]._1_2.one[*From]) == BAD_CHAR) {	\
		if(*xpgflg)						\
			/* see read_ccodes() and XLT_2 */		\
			pxlch = XPG_default;				\
		else							\
			pxlch = &Default1[cd];				\
	}

/*
***************************************************************************
** Translate double-byte character: translated 2-byte character obtained by:
**	1. get sequence numbers for 1st and 2nd bytes by indexing into the
**	   offset table section with the input character.
**	2. use sequence numbers as index to get the address of the translated 
**	   character.
** If there is no corresponding translated character for the input character, 
** use the default character.
** If any sequence number is bad, then the input character does not belong
** to the converted "from" code set.  Point to the input character that caused
** this condition and leave.
**	pxlch : pointer to the translated character
**	seq1 : sequence number of the 1st byte of 2-byte character
**	seq2 : sequence number of the 2nd byte of 2-byte character
***************************************************************************
*/

#define XLT_2( pxlch, seq1, seq2)					\
	seq1 = Table[cd]._1_2.fof2[*From];				\
	seq2 = Table[cd]._1_2.sof2[*(From+1)];				\
	if (seq1 == BAD_SEQ || seq2 == BAD_SEQ) {			\
		*inchar = From;						\
		*inbytesleft = Frombytes;				\
		if(*xpgflg) {						\
			errno = EILSEQ;					\
			RETURN BAD;					\
		} else							\
			RETURN BAD_IN_CHAR;				\
	}								\
	pxlch = Table[cd]._1_2.two[seq1][seq2];				\
	if ((*pxlch == BAD_CHAR) && (*(pxlch+1) == BAD_CHAR)) {		\
		if(*xpgflg)						\
			/* see read_ccodes() and XLT_1 */		\
			pxlch = XPG_default;				\
		else							\
			pxlch = &Default2[cd][0];			\
	}

/*
***************************************************************************
** Handle lock-shift information for JIS C6226.
***************************************************************************
*/

#define START_JIS( nomap)						\
	/* be sure kanji-out is not split on buffer boundary */		\
	if ((*inbytesleft < cs0->len) &&				\
	    (!strncmp( *inchar, cs0->str, *inbytesleft))) {		\
		if(*xpgflg) {						\
			errno = EINVAL;					\
			RETURN BAD;					\
		} else							\
			RETURN IN_CHAR_TRUNC;				\
	}								\
									\
	/* be sure kanji-in is not split on buffer boundary */		\
	if ((*inbytesleft < cs1->len) && 				\
	     (!strncmp( *inchar, cs1->str, *inbytesleft))) {		\
		if(*xpgflg) {						\
			errno = EINVAL;					\
			RETURN BAD;					\
		} else							\
			RETURN IN_CHAR_TRUNC;				\
	}								\
									\
	/* be sure kanji2-in is not split on buffer boundary */		\
	if ((*inbytesleft < cs2->len) &&				\
	    (!strncmp( *inchar, cs2->str, *inbytesleft))) {		\
		if(*xpgflg) {						\
			errno = EINVAL;					\
			RETURN BAD;					\
		} else							\
			RETURN IN_CHAR_TRUNC;				\
	}								\
									\
	/* handle any lockshift information */				\
	if (!strncmp( *inchar, cs0->str, cs0->len)) {			\
		/* get kanji-out lockshift and change the state */	\
		MOVEIN( cs0->len)					\
		cs->state = SINGLE_BYTE;				\
	}								\
	else if (!strncmp( *inchar, cs1->str, cs1->len)) {		\
		/* get kanji-in lockshift and change the state */	\
		MOVEIN( cs1->len)					\
		cs->state = KANJI;					\
	}								\
	else if (!strncmp( *inchar, cs2->str, cs2->len)) {		\
		/* get kanji2-in lockshift and change the state */	\
		MOVEIN( cs2->len)					\
		cs->state = KANJI2;					\
		if (nomap) {						\
			/* do not know how to map kanji2 */		\
			/* ... just move everything to output */	\
			MOVEOUT( i, cs2->str, cs2->len)			\
		}							\
	}

/*
***************************************************************************
** Check to see if input character (From) is a valid JIS character.
** If it is, then continue.
** If it is not, then point to the invalid input character and leave.
***************************************************************************
*/

#define IS_JIS(c)	(0x21 <= (c) && (c) <= 0x7e)

#define GOOD_JIS							\
	if (! (IS_JIS( From[0]) && IS_JIS( From[1])) ) {		\
		*inchar = From;						\
		*inbytesleft = Frombytes;				\
		if(*xpgflg) {						\
			errno = EINVAL;					\
			RETURN BAD;					\
		} else							\
			RETURN BAD_IN_CHAR;				\
	}

/*
***************************************************************************
** Check to see if input character (From) is a valid Shift-JIS character.
** If it is, then continue.
** If it is not, then point to the invalid input character and leave.
***************************************************************************
*/

#define GOOD_SJIS							\
	if (! (Secof2[cd][From[1]]) ) {					\
		*inchar = From;						\
		*inbytesleft = Frombytes;				\
		if(*xpgflg) {						\
			errno = EINVAL;					\
			RETURN BAD;					\
		} else							\
			RETURN BAD_IN_CHAR;				\
	}
/*
***************************************************************************
** Return EUC set number.
**	c : current input character
***************************************************************************
*/

#define EUC_SET(c)	(HIBITSET((c)))					\
			? (((c) == SS3) ? 3 : (((c) == SS2) ? 2 : 1))	\
			: 0

/*
***************************************************************************
** ICONV1 translation routines
***************************************************************************
*/

/*
***************************************************************************
** single-byte <--> single-byte
** Assumes 1-to-1 mapping in translation table.  No default characters.
***************************************************************************
*/

static int
ind_one_one( cd, to, from, buflen)
iconvd cd;				/* conversion descriptor */
UCHAR *to;				/* output buffer */
UCHAR *from;				/* input buffer */
int buflen;				/* length in bytes of input buffer */
{
	register int i;			/* counter */

	register UCHAR *t = &Table[cd]._1_2.one[0]; /* ptr to xlate table */

	for (i = buflen ; i > 0 ; i-- ) {
		*to++ = t[*from++];
	}
	return buflen;			/* return num bytes processed */
}

/*
***************************************************************************
** HP-15 <--> EBCDIC
** Assumes a default character may be necessary.
***************************************************************************
*/

static int
ind_one_two( cd, to, from, buflen)
iconvd cd;				/* conversion descriptor */
UCHAR *to;				/* output buffer */
UCHAR *from;				/* input buffer */
int buflen;				/* length in bytes of input buffer */
{
	register int i;			/* counter */
	register int c;			/* table character */

	register UCHAR *t = &Table[cd]._1_2.one[0]; /* ptr to xlate table */

	for (i = buflen ; i > 0 ; i-- ) {
		if ((c = t[*from++]) == BAD_CHAR) {
			if(XPG_flag[cd])
				*to++ = XPG_default;	/* default char */
			else
				*to++ = Default1[cd];	/* default char */
		}
		else {
			*to++ = c;			/* xlate char */
		}
	}
	return buflen;			/* return num bytes processed */
}

/*
***************************************************************************
** ICONV2 translation routines
***************************************************************************
*/

/*
***************************************************************************
** HP-15 <--> EBCDIC
***************************************************************************
*/

static int
ind_two_two( cd, to, from, buflen)
iconvd cd;				/* conversion descriptor */
UCHAR *to;				/* output buffer */
UCHAR *from;				/* input buffer */
int buflen;				/* length in bytes of input buffer */
{
	register int i;			/* counter */
	register int seq1, seq2;	/* sequence numbers */
	register UCHAR *pxlch;		/* ptr to translated char */

	for (i=buflen ; i > 0 ; i-=2 ) {

		/* get sequence numbers */
		seq1 = Table[cd]._1_2.fof2[*from++];
		seq2 = Table[cd]._1_2.sof2[*from++];
		if (seq1 == BAD_SEQ || seq2 == BAD_SEQ) {
			return BAD;
		}

		/* get translated character */
		pxlch = Table[cd]._1_2.two[seq1][seq2];
		if ((*pxlch == BAD_CHAR) && (*(pxlch+1) == BAD_CHAR)) {
			if(XPG_flag[cd])
				pxlch = XPG_default;
			else
				pxlch = &Default2[cd][0];
		}

		/* place xlated char in output buffer */
		*to++ = *pxlch++;
		*to++ = *pxlch;
	}
	return buflen;			/* return num bytes processed */
}

/*
***************************************************************************
** ICONV translation routines
***************************************************************************
*/

/*
***************************************************************************
** Single-byte translation routine.  All single-byte code sets
** using ICONV are translated here.
** Assumes all characters are mapped: no default character is used.
***************************************************************************
*/

static int
one_one( cd, inchar, inbytesleft, outchar, outbytesleft)
iconvd cd;				/* conversion descriptor */
UCHAR **inchar;				/* input buffer character */
int *inbytesleft;			/* num bytes left in input buffer */
UCHAR **outchar;			/* output buffer character */
int *outbytesleft;			/* num bytes left in output buffer */
{ 
	register int i;			/* a counter for MOVEOUT */
	register UCHAR *pxlch;		/* ptr to the translated character */
	register UCHAR *xpgflg = &XPG_flag[cd]; /* points to XPG_flag */
	register ccount = 0;

#ifdef _THREAD_SAFE
        _rec_mutex_lock(&_iconv_rmutex);
#endif
	while (*inbytesleft) {
		/* get character from input buffer */
		MOVEIN( 1)

		/* get the translated character from table */
		/* assume everything's mapped: no default character needed */
		pxlch = &Table[cd]._1_2.one[*From];

		/* put character into output buffer */
		MOVEOUT( i, pxlch, 1)

		/* for XPG4 return value */
		if(*pxlch != *From) ccount++;
	}
	if(*xpgflg)
		RETURN(ccount);
	else
		RETURN GOOD;
}

/*
***************************************************************************
***************************************************************************
*/

static int
hp15_bcd( cd, inchar, inbytesleft, outchar, outbytesleft)
iconvd cd;				/* conversion descriptor */
UCHAR **inchar;				/* input buffer character */
int *inbytesleft;			/* num bytes left in input buffer */
UCHAR **outchar;			/* output buffer character */
int *outbytesleft;			/* num bytes left in output buffer */
{ 
	register iconvinfo *cs = &cs_out[cd];	/* ptr to shift & state info */
	register iconvele *cs0 = &cs_out[cd].LS0;	/* lockshift-in */
	register iconvele *cs1 = &cs_out[cd].LS1;	/* lockshift-out */
	register int i;				/* a counter for MOVEOUT */
	register int seq1, seq2;		/* a sequence num for XLT_2 */
	register UCHAR *pxlch;			/* ptr to the xlated char */
	register UCHAR *xpgflg = &XPG_flag[cd]; /* points to XPG_flag */
	register ccount = 0;

#ifdef _THREAD_SAFE
        _rec_mutex_lock(&_iconv_rmutex);
#endif

	while (*inbytesleft) {

		if (Firstof2[cd][**inchar]) {
			/* get character from input buffer */
			MOVEIN( 2)

			/* get the translated character from table */
			XLT_2( pxlch, seq1, seq2)

			if (cs->state != DOUBLE_BYTE) {
				/* put out lock-shift */
				MOVEOUT( i, cs1->str, cs1->len)
				/* change the state */
				cs->state = DOUBLE_BYTE;
			}

			/* put 2-byte char into output buffer */
			MOVEOUT( i, pxlch, 2)

			/* for XPG4 return values */
			if(*pxlch != *From || *(pxlch+1) != *(From+1))
				ccount++;
		}
		else {
			/* get character from input buffer */
			MOVEIN( 1)

			/* get the translated character from table */
			XLT_1( pxlch)

			if (cs->state != SINGLE_BYTE) {
				/* put out lock-shift */
				MOVEOUT( i, cs0->str, cs0->len)
				/* change the state */
				cs->state = SINGLE_BYTE;
			}

			/* put 1-byte char into output buffer */
			MOVEOUT( i, pxlch, 1)

			/* for XPG4 return values */
			if(*pxlch != *From) ccount++;
		}
	}
	if(*xpgflg)
		RETURN(ccount);
	else
		RETURN GOOD;
}

/*
***************************************************************************
***************************************************************************
*/

static int
hp15_hp15( cd, inchar, inbytesleft, outchar, outbytesleft)
iconvd cd;				/* conversion descriptor */
UCHAR **inchar;				/* input buffer character */
int *inbytesleft;			/* num bytes left in input buffer */
UCHAR **outchar;			/* output buffer character */
int *outbytesleft;			/* num bytes left in output buffer */
{ 
#ifdef BOZO
	register iconvinfo *cs = &cs_out[cd];	/* ptr to shift & state info */
	register iconvele *cs0 = &cs_out[cd].LS0;	/* lockshift-in */
	register iconvele *cs1 = &cs_out[cd].LS1;	/* lockshift-out */
#endif
	register int i;				/* a counter for MOVEOUT */
	register int seq1, seq2;		/* a sequence num for XLT_2 */
	register UCHAR *pxlch;			/* ptr to the xlated char */
	register UCHAR *xpgflg = &XPG_flag[cd]; /* points to XPG_flag */
	register ccount = 0;

#ifdef _THREAD_SAFE
        _rec_mutex_lock(&_iconv_rmutex);
#endif
	while (*inbytesleft) {

		if (Firstof2[cd][**inchar]) {
			/* get character from input buffer */
			MOVEIN( 2)

			/* get the translated character from table */
			XLT_2( pxlch, seq1, seq2)

			/* put 2-byte char into output buffer */
			MOVEOUT( i, pxlch, 2)

			/* for XPG4 return values */
			if(*pxlch != *From || *(pxlch+1) != *(From+1))
				ccount++;
		}
		else {
			/* get character from input buffer */
			MOVEIN( 1)

			/* get the translated character from table */
			XLT_1( pxlch)

			/* put 1-byte char into output buffer */
			MOVEOUT( i, pxlch, 1)

			/* for XPG4 return values */
			if(*pxlch != *From) ccount++;
		}
	}
	if(*xpgflg)
		RETURN(ccount);
	else
		RETURN GOOD;
}

/*
***************************************************************************
***************************************************************************
*/

static int
bcd_hp15( cd, inchar, inbytesleft, outchar, outbytesleft)
iconvd cd;				/* conversion descriptor */
UCHAR **inchar;				/* input buffer character */
int *inbytesleft;			/* num bytes left in input buffer */
UCHAR **outchar;			/* output buffer character */
int *outbytesleft;			/* num bytes left in output buffer */
{ 
	register iconvinfo *cs = &cs_in[cd];	/* ptr to shift & state info */
	register iconvele *cs0 = &cs_in[cd].LS0;	/* lockshift-in */
	register iconvele *cs1 = &cs_in[cd].LS1;	/* lockshift-out */
	register int i;				/* a counter for MOVEOUT */
	register int seq1, seq2;		/* a sequence num for XLT_2 */
	register UCHAR *pxlch;			/* ptr to the xlated char */
	register UCHAR *xpgflg = &XPG_flag[cd]; /* points to XPG_flag */
	register ccount = 0;

#ifdef _THREAD_SAFE
        _rec_mutex_lock(&_iconv_rmutex);
#endif

	while (*inbytesleft) {

		/* be sure single-byte-in is not split on buffer boundary */
		if ((*inbytesleft < cs0->len) && (!strncmp( *inchar, cs0->str, *inbytesleft))) {
			if(*xpgflg) {
				errno = EINVAL;
				RETURN BAD;
			} else
				RETURN IN_CHAR_TRUNC;
		}

		/* be sure double-byte-in is not split on buffer boundary */
		if ((*inbytesleft < cs1->len) && (!strncmp( *inchar, cs1->str, *inbytesleft))) {
			if(*xpgflg) {
				errno = EINVAL;
				RETURN BAD;
			} else
				RETURN IN_CHAR_TRUNC;
		}

		/* handle any lockshift information */
		if (!strncmp( *inchar, cs0->str, cs0->len)) {
			/* change the state and get lockshift-in */
			MOVEIN( cs0->len)
			cs->state = SINGLE_BYTE;
		}
		else if (!strncmp( *inchar, cs1->str, cs1->len)) {
			/* change the state and get lockshift-out */
			MOVEIN( cs1->len)
			cs->state = DOUBLE_BYTE;

		}
		else {

			/* do the translation */
			if ( cs->state == DOUBLE_BYTE) {
				/* get character from input buffer */
				MOVEIN( 2)

				/* get the translated character from table */
				XLT_2( pxlch, seq1, seq2)

				/* put character into output buffer */
				MOVEOUT( i, pxlch, 2)

				/* for XPG4 return values */
				if(*pxlch != *From || *(pxlch+1) != *(From+1))
					ccount++;
			}
			else {
				/* get character from input buffer */
				MOVEIN( 1)

				/* get the translated character from table */
				XLT_1( pxlch)

				/* put character into output buffer */
				MOVEOUT( i, pxlch, 1)

				/* for XPG4 return values */
				if(*pxlch != *From) ccount++;
			}
		}
	}
	if(*xpgflg)
		RETURN(ccount);
	else
		RETURN GOOD;
}

/*
***************************************************************************
***************************************************************************
*/

static int
sjis_jis( cd, inchar, inbytesleft, outchar, outbytesleft)
iconvd cd;				/* conversion descriptor */
UCHAR **inchar;				/* input buffer character */
int *inbytesleft;			/* num bytes left in input buffer */
UCHAR **outchar;			/* output buffer character */
int *outbytesleft;			/* num bytes left in output buffer */
{ 
	extern char *csjtojis();	/* shift-jis to jis routine */

	register iconvinfo *cs = &cs_out[cd];	/* ptr to shift & state info */
	register iconvele *cs0 = &cs_out[cd].LS0;	/* kanji-out */
	register iconvele *cs1 = &cs_out[cd].LS1;	/* kanji-in */
	register int i;				/* a counter for MOVEOUT */
	register UCHAR *xpgflg = &XPG_flag[cd]; /* points to XPG_flag */
	register ccount = 0;

#ifdef _THREAD_SAFE
        _rec_mutex_lock(&_iconv_rmutex);
#endif

	while (*inbytesleft) {

		if (Firstof2[cd][**inchar]) {
			/* get character from input buffer */
			MOVEIN( 2)

			/* be sure it's a good sjis char */
			GOOD_SJIS

			/* get the translated jis character from routine */
			(void) csjtojis( Jis, From);

			/* if necessary, put out lockshift */
			if (cs->state != KANJI) {
				MOVEOUT( i, cs1->str, cs1->len)
				cs->state = KANJI;
			}

			/* put 2-byte char into output buffer */
			MOVEOUT( i, Jis, 2)

			/* for XPG4 return values */
			if(*Jis != *From || *(Jis+1) != *(From+1))
				ccount++;
		}
		else {
			/* get character from input buffer */
			MOVEIN( 1)

			/* if necessary, put out lockshift */
			if (cs->state != SINGLE_BYTE) {
				MOVEOUT( i, cs0->str, cs0->len)
				cs->state = SINGLE_BYTE;
			}

			/* put 1-byte char into output buffer */
			MOVEOUT( i, From, 1)
		}
	}
	if(*xpgflg)
		RETURN(ccount);
	else
		RETURN GOOD;
}

/*
***************************************************************************
***************************************************************************
*/

static int
sjis_ujis( cd, inchar, inbytesleft, outchar, outbytesleft)
iconvd cd;				/* conversion descriptor */
UCHAR **inchar;				/* input buffer character */
int *inbytesleft;			/* num bytes left in input buffer */
UCHAR **outchar;			/* output buffer character */
int *outbytesleft;			/* num bytes left in output buffer */
{ 
	extern char *csjtouj();		/* shift-jis to ujis routine */

	register int i;			/* a counter for MOVEOUT */
	register UCHAR *xpgflg = &XPG_flag[cd]; /* points to XPG_flag */
	register ccount = 0;

#ifdef _THREAD_SAFE
        _rec_mutex_lock(&_iconv_rmutex);
#endif
	while (*inbytesleft) {

		if (Firstof2[cd][**inchar]) {
			/* get character from input buffer */
			MOVEIN( 2)

			/* be sure it's a good sjis char */
			GOOD_SJIS

			/* get the translated ujis character from routine */
			(void) csjtouj( Ss1, From);

			/* set1 kanji: 2 8-bit into output */
			MOVEOUT( i, Ss1, 2)

			/* for XPG4 return values */
			if(*Ss1 != *From || *(Ss1+1) != *(From+1))
				ccount++;
		}
		else {
			/* get character from input buffer */
			MOVEIN( 1)

			if (HIBITSET( *From)) {
				/* set 2 katakana: SS2 + 8-bit into output */
				Ss2[1] = *From;
				MOVEOUT( i, Ss2, 2)

				/* for XPG4 return value */
				ccount++;
			}
			else {
				/* set 0 ascii: 1 7-bit into output */
				MOVEOUT( i, From, 1)
			}
		}
	}
	if(*xpgflg)
		RETURN(ccount);
	else
		RETURN GOOD;
}

/*
***************************************************************************
***************************************************************************
*/

static int
ujis_jis( cd, inchar, inbytesleft, outchar, outbytesleft)
iconvd cd;				/* conversion descriptor */
UCHAR **inchar;				/* input buffer character */
int *inbytesleft;			/* num bytes left in input buffer */
UCHAR **outchar;			/* output buffer character */
int *outbytesleft;			/* num bytes left in output buffer */
{ 
	register iconvinfo *cs = &cs_out[cd];	/* ptr to shift & state info */
	register iconvele *cs0 = &cs_out[cd].LS0;	/* kanji-out */
	register iconvele *cs1 = &cs_out[cd].LS1;	/* kanji-in */
	register iconvele *cs2 = &cs_out[cd].LS2;	/* kanji2-in */
	register int i;				/* a counter for MOVEOUT */
	register UCHAR *xpgflg = &XPG_flag[cd]; /* points to XPG_flag */
	register ccount = 0;

#ifdef _THREAD_SAFE
        _rec_mutex_lock(&_iconv_rmutex);
#endif

	while (*inbytesleft) {

		switch (EUC_SET(**inchar)) {
		case 0:
			/* UJIS set 0: ascii */
			MOVEIN( 1)

			/* if necessary, put out lockshift */
			if (cs->state != SINGLE_BYTE) {
				MOVEOUT( i, cs0->str, cs0->len)
				cs->state = SINGLE_BYTE;
			}

			/* put 1-byte acii into output buffer */
			MOVEOUT( i, From, 1)
			break;
		case 1:
			/* UJIS set 1: kanji */
			MOVEIN( 2)

			/* if necessary, put out lockshift */
			if (cs->state != KANJI) {
				MOVEOUT( i, cs1->str, cs1->len)
				cs->state = KANJI;
			}

			/* turn off the hi bits */
			Ss1[0] = HIBITOFF( *From);
			Ss1[1] = HIBITOFF( *(From+1));

			/* put 2-byte kanji into output buffer */
			MOVEOUT( i, Ss1, 2)

			/* for XPG4 return values */
			if(Ss1[0] != *From || Ss1[1] != *(From+1))
				ccount++;
			break;

		case 2:	
			/* UJIS set 2: katakana */
			MOVEIN( 2)

			/* if necessary, put out lockshift */
			if (cs->state != SINGLE_BYTE) {
				MOVEOUT( i, cs0->str, cs0->len)
				cs->state = SINGLE_BYTE;
			}

			/* put 1-byte katakana into output buffer */
			MOVEOUT( i, From+1, 1)

			/* for XPG4 return values */
			ccount++;
			break;

		case 3:
			/* UJIS set 3: gaji */
			MOVEIN( 3)

			/* if necessary, put out lockshift */
			if (cs->state != KANJI2) {
				MOVEOUT( i, cs2->str, cs2->len)
				cs->state = KANJI2;
			}

			/* turn off the hi bits */
			Ss1[0] = HIBITOFF( *(From+1));
			Ss1[1] = HIBITOFF( *(From+2));

			/* put 2-byte gaji into output buffer */
			MOVEOUT( i, Ss1, 2)

			/* for XPG4 return values */
			ccount++;
		}
	}
	if(*xpgflg)
		RETURN(ccount);
	else
		RETURN GOOD;
}

/*
***************************************************************************
***************************************************************************
*/

static int
ujis_sjis( cd, inchar, inbytesleft, outchar, outbytesleft)
iconvd cd;				/* conversion descriptor */
UCHAR **inchar;				/* input buffer character */
int *inbytesleft;			/* num bytes left in input buffer */
UCHAR **outchar;			/* output buffer character */
int *outbytesleft;			/* num bytes left in output buffer */
{ 
	extern char *cujtosj( );	/* ujis to shift-jis routine */
	register int i;			/* a counter for MOVEOUT */
	register UCHAR *xpgflg = &XPG_flag[cd]; /* points to XPG_flag */
	register ccount = 0;

#ifdef _THREAD_SAFE
        _rec_mutex_lock(&_iconv_rmutex);
#endif
	while (*inbytesleft) {

		switch (EUC_SET(**inchar)) {
		case 0:
			/* UJIS set 0: ascii */
			MOVEIN( 1)

			/* just move it to output buffer */
			MOVEOUT( i, From, 1)
			break;

		case 1:
			/* UJIS set 1: kanji */
			MOVEIN( 2)

			/* translate to ujis to shift-jis */
			(void) cujtosj( Sjis, From);

			/* move shift-jis to output buffer */
			MOVEOUT( i, Sjis, 2)
			break;

			/* for XPG4 return values */
			if(*Sjis != *From || *(Sjis+1) != *(From+1))
				ccount++;

		case 2:	
			/* UJIS set 2: katakana */
			MOVEIN( 2)

			/* move it to output buffer without SS2 */
			MOVEOUT( i, From+1, 1)

			/* for XPG4 return value */
			ccount++;
			break;
		case 3:
			/* UJIS set 3: gaji */
			/* Do not know how to map these characters into 
			   ... shift-jis, so move them to output buffer 
			   ... with the SS3 */
			MOVEIN( 3)
			MOVEOUT( i, From, 3)
		}
	}
	if(*xpgflg)
		RETURN(ccount);
	else
		RETURN GOOD;
}

/*
***************************************************************************
***************************************************************************
*/

static int
jis_sjis( cd, inchar, inbytesleft, outchar, outbytesleft)
iconvd cd;				/* conversion descriptor */
UCHAR **inchar;				/* input buffer character */
int *inbytesleft;			/* num bytes left in input buffer */
UCHAR **outchar;			/* output buffer character */
int *outbytesleft;			/* num bytes left in output buffer */
{ 
	extern char *cjistosj();		/* jis to shift-jis routine */
	register iconvinfo *cs = &cs_in[cd];	/* ptr to shift & state info */
	register iconvele *cs0 = &cs_in[cd].LS0;	/* jis kanji-out */
	register iconvele *cs1 = &cs_in[cd].LS1;	/* jis kanji-in */
	register iconvele *cs2 = &cs_in[cd].LS2;	/* jis kanji2-in */
	register int i;				/* a counter for MOVEOUT */
	register UCHAR *xpgflg = &XPG_flag[cd]; /* points to XPG_flag */
	register ccount = 0;

#ifdef _THREAD_SAFE
        _rec_mutex_lock(&_iconv_rmutex);
#endif

	while (*inbytesleft) {

		/* scan past lockshift and set state */
		START_JIS( TRUE)

		/* do the translation */
		switch (cs->state) {
		case START:	
		case SINGLE_BYTE:	
			/* get 1-byte jis char from input buffer */
			MOVEIN( 1)

			/* pass thru single byte charaters as is */
			MOVEOUT( i, From, 1)
			break;

		case KANJI:	
			/* get 2-byte jis char from input buffer */
			MOVEIN( 2)

			/* be sure it's a good jis char */
			GOOD_JIS

			/* translate to shift-jis */
			(void) cjistosj( Sjis, From);

			/* put 2-byte shift-jis char into output buffer */
			MOVEOUT ( i, Sjis, 2)

			/* for XPG4 return values */
			if(*Sjis != *From || *(Sjis+1) != *(From+1))
				ccount++;
			break;

		case KANJI2:	
			/* get 2-byte jis level III char from input buffer */
			MOVEIN( 2)

			/* be sure it's a good jis char */
			GOOD_JIS

			/* pass thru jis level III charaters as is */
			MOVEOUT ( i, From, 2)
			break;
		}
	}
	if(*xpgflg)
		RETURN(ccount);
	else
		RETURN GOOD;
}

/*
***************************************************************************
***************************************************************************
*/

static int
jis_ujis( cd, inchar, inbytesleft, outchar, outbytesleft)
iconvd cd;				/* conversion descriptor */
UCHAR **inchar;				/* input buffer character */
int *inbytesleft;			/* num bytes left in input buffer */
UCHAR **outchar;			/* output buffer character */
int *outbytesleft;			/* num bytes left in output buffer */
{ 
	register iconvinfo *cs = &cs_in[cd];	/* ptr to shift & state info */
	register iconvele *cs0 = &cs_in[cd].LS0;	/* jis kanji-out */
	register iconvele *cs1 = &cs_in[cd].LS1;	/* jis kanji-in */
	register iconvele *cs2 = &cs_in[cd].LS2;	/* jis kanji2-in */
	register int i;				/* a counter for MOVEOUT */
	register UCHAR *xpgflg = &XPG_flag[cd]; /* points to XPG_flag */
	register ccount = 0;

#ifdef _THREAD_SAFE
        _rec_mutex_lock(&_iconv_rmutex);
#endif

	while (*inbytesleft) {

		/* scan past lockshift and set state */
		START_JIS( FALSE)

		/* do the translation */
		switch (cs->state) {
		case START:	
		case SINGLE_BYTE:	
			/* get 1-byte jis character from input buffer */
			MOVEIN( 1)

			if (HIBITSET( *From)) {
				/* set 2 katakana: SS2 + 8-bit into output */
				Ss2[1] = *From;
				MOVEOUT( i, Ss2, 2)

				/* for XPG4 return value */
				ccount++;
			}
			else {
				/* set 0 ascii: 1 7-bit into output */
				MOVEOUT( i, From, 1)
			}
			break;

		case KANJI:	
			/* get 2-byte jis character from input buffer */
			MOVEIN( 2)

			/* be sure it's a good jis char */
			GOOD_JIS

			/* flip on high bits to get ujis from jis */
			Ss1[0] = HIBITON( *From);
			Ss1[1] = HIBITON( *(From+1));

			/* set 1 kanji: 2 8-bit into output */
			MOVEOUT ( i, Ss1, 2)

			/* for XPG4 return values */
			if(*Ss1 != *From || *(Ss1+1) != *(From+1))
				ccount++;
			break;

		case KANJI2:	
			/* get 2-byte jis III character from input buffer */
			MOVEIN( 2)

			/* be sure it's a good jis char */
			GOOD_JIS

			/* flip on high bits to get ujis from jis */
			Ss3[1] = HIBITON( *From);
			Ss3[2] = HIBITON( *(From+1));

			/* set 3 gaji: SS3 + 2 8-bit into output */
			MOVEOUT ( i, Ss3, 3)

			/* for XPG4 return values */
			ccount++;
			break;
		}
	}
	if(*xpgflg)
		RETURN(ccount);
	else
		RETURN GOOD;
}

/*
**************************************************************************
** do nothing routine.
**************************************************************************
*/

static int
no_op( )
{
	return GOOD;
}

/*
**************************************************************************
** Initialization Routines
**************************************************************************
*/

/*
**************************************************************************
**************************************************************************
*/

static int
init_bcd( cd, whichway)
iconvd cd;
int whichway;
{
	if (iconvlock( cd, whichway, 0, SI) == BAD) {
		return BAD;
	}
	if (iconvlock( cd, whichway, 1, SO) == BAD) {
		return BAD;
	}
	return GOOD;
}

/*
**************************************************************************
**************************************************************************
*/

static int
init_hp15( cd, whichway)
iconvd cd;
int whichway;
{
	if (whichway == FROM) {
		char lcfn[MAX_NAME];		/* locale.inf filename */
		char lchdr[TBL_SIZE];		/* locale header */
		char lcdata[_LC_CTYPE_SIZE];	/* LC_CYTPE data */
		struct table_header *table_hdr;	/* locale header ptr */
		struct ctype_header *ctype_hdr;	/* ctype category ptr */
		struct catinfotype *cat_hdr_ptr;/* category-modifer ptr */
		int fd;				/* file desc of locale.inf */
		int header_size;		/* size of header */
		int cat_mod_size;		/* size of category-modifier */
		register unsigned char *p1;	/* 1st-of-2 ptr */
		register unsigned char *p2;	/* 2nd-of-2 ptr */
		register int i;			/* counter */

		/* get locale.inf filename & open it*/
		(void) strcpy( lcfn, NLSDIR);
		(void) strcat( lcfn, Fromlangname);
		(void) strcat( lcfn, TBL_NAME);
		if ((fd = open( lcfn, O_RDONLY)) < 0) {
			return BAD;
		}

		/* read locale table header */
		if (HDR_SIZE < 1 || HDR_SIZE > TBL_SIZE ||
			read( fd, lchdr, HDR_SIZE) < HDR_SIZE) {
			close(fd);
			return BAD;
		}
		table_hdr = (struct table_header *)lchdr;

		/* get size of header and category-modifier structures */
		header_size = table_hdr->size;
		cat_mod_size = (table_hdr->cat_no + table_hdr->mod_no) * sizeof( struct catinfotype);

		/* read locale category-modifier structures */
		if (cat_mod_size < 1 || cat_mod_size > TBL_SIZE ||
			read( fd, lchdr, cat_mod_size) < cat_mod_size) {
			close(fd);
			return BAD;
		}
		cat_hdr_ptr = (struct catinfotype *)(lchdr + LC_CTYPE * sizeof( struct catinfotype));

		/* be sure there is a sensible LC_CTYPE locale section */
		if (cat_hdr_ptr->size < 1 || cat_hdr_ptr->size > _LC_CTYPE_SIZE) {
			close(fd);
			return BAD;
		}

		/* go to the LC_CTYPE locale section and read it in */
		lseek( fd, cat_hdr_ptr->address + header_size, 0);
		if (read( fd, lcdata, cat_hdr_ptr->size) < cat_hdr_ptr->size) {
			close(fd);
			return BAD;
		}
		ctype_hdr = (struct ctype_header *)lcdata;

		/* be sure the firstof2 and secof2 offsets make sense */
		if (ctype_hdr->kanji1_addr < 0 || ctype_hdr->kanji1_addr >= _LC_CTYPE_SIZE) {
			close(fd);
			return BAD;
		}
		if (ctype_hdr->kanji2_addr < 0 || ctype_hdr->kanji2_addr >= _LC_CTYPE_SIZE) {
			close(fd);
			return BAD;
		}

		/* find firstof2 and secof2 pointers */
		p1 = (unsigned char *) lcdata + ctype_hdr->kanji1_addr+1;
		p2 = (unsigned char *) lcdata + ctype_hdr->kanji2_addr+1;

		/* copy 1st-of-2 and 2nd-of-2table */
		for (i=0 ; i < 256 ; i++) {
			Firstof2[cd][i] = p1[i];
			Secof2[cd][i] = p2[i];
		}
		if (close(fd) < 0)
			return BAD;
		return GOOD;
	}
	else if (whichway == TO) {
		return GOOD;		/* nothing needed for output */
	}
	else {
		return BAD;
	}
}

/*
**************************************************************************
**************************************************************************
*/

static int
init_sjis( cd, whichway)
iconvd cd;
int whichway;
{
	if (whichway == FROM ) {
		strcpy( Fromlangname, "japanese");
		if (init_hp15( cd, FROM) == BAD) {
			return BAD;
		}
		return GOOD;
	}
	else if (whichway == TO ) {
		return GOOD;		/* nothing needed for output */
	}
	else {
		return BAD;
	}
}

/*
**************************************************************************
**************************************************************************
*/

static int
init_jis( cd, whichway)
iconvd cd;
int whichway;
{
	if (iconvlock( cd, whichway, 0, KO) == BAD) {
		return BAD;
	}
	if (iconvlock( cd, whichway, 1, KI) == BAD) {
		return BAD;
	}
	if (iconvlock( cd, whichway, 2, KI2) == BAD) {
		return BAD;
	}
	return GOOD;
}

/*
**************************************************************************
**************************************************************************
*/

static int
init_euc( cd, whichway)
iconvd cd;
int whichway;
{
	register iconvinfo *cs;		/* code set structure */

	if (whichway == FROM ) {
		cs = &cs_in[cd];	/* ptr to input shift & state info */
	}
	else if (whichway == TO ) {
		cs = &cs_out[cd];	/* ptr to output shift & state info */
	}
	else {
		return BAD;
	}

	cs->setlen[0] = 1;		/* length of euc sets */
	cs->setlen[1] = Euc_setlen[0];
	cs->setlen[2] = Euc_setlen[1];
	cs->setlen[3] = Euc_setlen[2];

	return GOOD;
}

/*
**************************************************************************
** Scan routines used by read_header() (scanf not used to save space).
**************************************************************************
*/

/*
**************************************************************************
** Return a number if the next item in the table header is a number.
** Otherwise, return BAD.
**************************************************************************
*/

static int
get_num( p1, p2)
char **p1;			/* ptr to ptr to beginning of number */
char **p2;			/* ptr to ptr to end of number from strtol */
{
	extern long strtol();

	int i;

	if (((i = (int) strtol( *p1, p2, 10)) == 0) && (*p1 == *p2)) {
		return BAD;
	}
	return i;
}

/*
**************************************************************************
** Return length of next non-space item.
**************************************************************************
*/

static int
get_name( p1, p2)
char **p1;			/* ptr to ptr to beginning of name */
char **p2;			/* ptr to ptr to end of name */
{
	/* skip leading spaces to find start of name */
	for (*p2 = *p1 ; isspace(**p2 & 0xff) ; (*p2)++) ;

	/* find end of name */
	for (*p1 = *p2 ; !isspace(**p2 & 0xff) ; (*p2)++) ;

	/* return the name length */
	return *p2 - *p1;
}

/*
**************************************************************************
** Return the next non-space character as a number.
**************************************************************************
*/

static int
get_char( p1, p2)
char **p1;			/* ptr to ptr to beginning of character */
char **p2;			/* ptr to ptr to end of character */
{
	/* skip leading spaces */
	for (*p2 = *p1 ; isspace(**p2 & 0xff) ; (*p2)++) ;

	/* point to the character and move 1 byte past it */
	*p1 = (*p2)++;

	/* return the character as an integer */
	return (int) (**p1 & 0xff);
}

/*
**************************************************************************
** Process translation table routines
**************************************************************************
*/

/*
**************************************************************************
** Return GOOD if table header read successfully and the stuff in it is
** valid.  Otherwise, return BAD.
** Header structure:
**	1. version number
**	2. languge name of "from" code set
**	3. languge name of "to" code set
**	4. numeric code of "from" code set
**	5. numeric code of "to" code set
** If HP15 or EBCDIC:
**	1. max number of columns in 1st-of-2 offset or sequence number table
**	2. max number of columns in 2nd-of-2 offset or sequence number table
** If EUC
**	1. length of set 1 	(set 0 always assumed to be 1)
**	2. length of set 2
**	3. length of set 3
**************************************************************************
*/

static int
read_header( fd)
int fd;					/* xlate table file descriptor */
{
	char header[HEADER_SIZE];	/* conversion table header */
	char *p1 = header;		/* ptr into table header */
	char *p2;			/* ptr into table header */
	int n;				/* number of bytes in a char array */

	/* read in the header */
	if (read( fd, header, HEADER_SIZE) < 0) {
		return BAD;
	}

	/* get & validate the version number */
	if ((n = get_num( &p1, &p2)) < 0 || n > LAST_VERSION) {
		return BAD;
	}

	/* get & validate the "from" language name */
	if ((n = get_name( &p2, &p1)) == 0 || n > MAX_NAME) {
		return BAD;
	}
	(void) strncpy( Fromlangname, p2, n); Fromlangname[n] = '\0';

	/* get & validate the "to" language name */
	if ((n = get_name( &p1, &p2)) == 0 || n > MAX_NAME) {
		return BAD;
	}
	(void) strncpy( Tolangname, p1, n); Tolangname[n] = '\0';

	/* get & validate the "from" language index */
	if ((Fromindex = get_num( &p2, &p1)) < FIRST_CLASS || Fromindex > LAST_CLASS) {
		return BAD;
	}

	/* get & validate the "to" language index */
	if ((Toindex = get_num( &p1, &p2)) < FIRST_CLASS || Toindex > LAST_CLASS) {
		return BAD;
	}

	if (Fromindex == HP15 || Fromindex == EBCDIC16) {
		/* get & validate num of 1st-of-2 & 2nd-of-2 in xlate table */
		/* This should have been an ASCII number fetched by get_num */
		if ((Fof2_max = (UCHAR) get_char( &p2, &p1)) < 1 || Fof2_max > 256) {
			return BAD;
		}
		if ((Sof2_max = (UCHAR) get_char( &p1, &p2)) < 1 || Sof2_max > 256) {
			return BAD;
		}
	}

	if (Fromindex == EUC || Toindex == EUC) {
		Euc_setlen[0] =  get_char( &p2, &p1);
		Euc_setlen[1] =  get_char( &p1, &p2);
		Euc_setlen[2] =  get_char( &p2, &p1);
	}
	
	return GOOD;
}

/*
**************************************************************************
** Return GOOD if direct translation table read successfully
** Otherwise, return BAD.
** Structure of translation tables:
**
** 	single-byte <--> single-byte
**	+----------------------+
**	| Header               |   64 byte.
**	+----------------------+
**	| 8bit conv table      |   256 byte.
**	+----------------------+
**
** 	HP-15 <--> EBCDIC
**	+----------------------+
**	| Header               |   64 byte.
**	+----------------------+
**	| Firstof2 table       |   512 byte.
**	+----------------------+
**	| Secof2 table         |   512 byte.
**	+----------------------+
**	| 8bit conv table      |   256 byte.
**	+----------------------+
**	| 16bit conv table     |   maxfof2 * maxsof2 * 2 byte.
**	+----------------------+ 
**
** All other table structures undefined.  The TBLTYPE is a union to allow
** other structures when they are defined (see typedefs above).
**************************************************************************
*/

static int
read_direct( cd, table_ptr)
iconvd cd;			/* conversion descriptor */
UCHAR *table_ptr;		/* where to put xlate table */
{
	register int fd;	/* file descriptor */
	register int i;		/* generic counter */
	register int row;	/* number of rows in Table[cd].1_2.two[i][j] */

	/* open the file, read and process the header */
	if ((fd = open( Toname, O_RDONLY)) < 0) {
		return BAD;
	}
	if (read_header( fd) == BAD) {
		return BAD;
	}

	/* don't need need a table for these conversions */
	if (Fromindex >= ALGO && Toindex >= ALGO) {
		return GOOD;
	}

	/* get conversion table */
	switch (Fromindex) {
	case HP15:
	case EBCDIC16:
		/* read in the 1st-of-2 sequence number table */
		if (read( fd, table_ptr, 256 * sizeof( UINT)) < 0) {
			return BAD;
		}
		Table[cd]._1_2.fof2 = (UINT * ) table_ptr;
		table_ptr += 256 * sizeof( UINT);

		/* read in the 2nd-of-2 sequence number table */
		if (read( fd, table_ptr, 256 * sizeof( UINT)) < 0) {
			return BAD;
		}
		Table[cd]._1_2.sof2 = (UINT* ) table_ptr;
		table_ptr += 256 * sizeof( UINT);

		/* read in the one-byte table */
		if (read( fd, table_ptr, 256) < 0) {
			return BAD;
		}
		Table[cd]._1_2.one = table_ptr;
		table_ptr += 256 * sizeof( UCHAR);

		/* read in the two-byte table */
		Table[cd]._1_2.two = (CH2 **) table_ptr;
		row = Sof2_max * sizeof( CH2);
		table_ptr += sizeof( CH2 *) * Fof2_max;
		for (i=0 ; i < Fof2_max ; i++, table_ptr += row) {
			if (read( fd, table_ptr, row) < 0) {
				return BAD;
			}
			Table[cd]._1_2.two[i] = (CH2 *) table_ptr;
		}
		break;

	case DIRECT8:
		/* read in the one-byte table */
		if (read( fd, table_ptr, 256) < 0) {
			return BAD;
		}
		Table[cd]._1_2.one = table_ptr;
		break;

	case TJIS:
	case EUC:
		/* table not defined yet */
		return BAD;

	case CCODES8:
	default:
		return BAD;
	}

	/* close conversion table and leave */
	if (close( fd) < 0) {
		return BAD;
	}
	
	return GOOD;
}

/*
**************************************************************************
** Codeset conversion table comparision routines use by bsearch(3c).
**************************************************************************
*/

static int
cmpchar( i, j)
CONVCODE *i;		/* ptr to 1st ele of conversion code xlate table */
CONVCODE *j;		/* ptr to 2nd ele of conversion code xlate table */
{
	if (i->char_code < j->char_code) {
		return -1;			/* less than */
	}
	else if (i->char_code > j->char_code) {	
		return 1;			/* greater than */
	}
	else {
		return 0;			/* equal to */
	}
}

static int
cmpcode( i, j)
CONVCODE *i;		/* ptr to 1st ele of conversion code xlate table */
CONVCODE *j;		/* ptr to 2nd ele of conversion code xlate table */
{
	if (i->conv_code < j->conv_code) {
		return -1;			/* less than */
	}
	else if (i->conv_code > j->conv_code) {	
		return 1;			/* greater than */
	}
	else {
		return 0;			/* equal to */
	}
}


/*
**************************************************************************
** Return GOOD if conversion code table read successfully.
** Otherwise, return BAD.
**************************************************************************
*/

static int
read_ccodes( cd, table_ptr)
iconvd cd;				/* conversion descriptor */
UCHAR *table_ptr	;		/* where to put xlate table */
{
	extern char *bsearch();		/* binary search */
	extern void qsort();		/* quick sort */

	CONVCODE from[SIZE_CCODES];	/* "from" conversion code set table */
	CONVCODE to[SIZE_CCODES];	/* "to" conversion code set table */
	CONVCODE *lookup;		/* result of binary search */
	CONVCODE defch;			/* default character */
	register int i;			/* counter */
	register int fd1;		/* file descriptor "from" file */
	register int fd2;		/* file descriptor "to" file */

	/* open the "converted from" file, read and process the header */
	if ((fd1 = open( Fromname, O_RDONLY)) < 0) {
		return BAD;
	}
	if (read_header( fd1) == BAD) {
		return BAD;
	}

	/* only one "converted from" conversion code set class */
	if (Fromindex != CCODES8) {
		return BAD;
	}

	/* open the "converted to" file, read and process the header */
	if ((fd2 = open( Toname, O_RDONLY)) < 0) {
		return BAD;
	}
	if (read_header( fd2) == BAD) {
		return BAD;
	}

	/* only one "converted to" conversion code set class */
	if ((Toindex = Fromindex) != CCODES8) {
		return BAD;
	}

	/* set up the default character */
	/* no concept of default character for XPG4 routines */
	if(!XPG_flag[cd]) {
		defch.conv_code = 0xffff;
		defch.char_code = Default1[cd];
	}

	/* Read "from" and "to" "conversion code set" tables */
	if (read ( fd1, (char *)&from[0], sizeof( CONVCODE)* SIZE_CCODES) < 0) {
		return BAD;
	}
	if (read ( fd2, (char *)&to[0], sizeof( CONVCODE)* SIZE_CCODES) < 0) {
		return BAD;
	}

	/* save start of conversion table */
	Table[cd]._1_2.one = table_ptr;

	/* sort "converted from" table on conversion character */
	qsort( (char*) &from[0], SIZE_CCODES, sizeof( CONVCODE), cmpchar);

	/* sort "converted to" table on conversion code */
	qsort( (char*) &to[0], SIZE_CCODES, sizeof( CONVCODE), cmpcode);

	/*
	** For each "conversion code" charcter in the "converted from" code set,
	** find the corresponding "conversion code" character
	** in the "converted to" code set and move the "converted to"
	** character into the conversion table.  If no corresponding
	** character then move a default character into conversion table.
	** Result: a table of "converted to" characters that can be indexed
	** by "converted from" characters.
	*/

	for (i=0; i < SIZE_CCODES ; i++) {
		if ((lookup = (CONVCODE*) bsearch( (char *)&from[i] , (char *)&to[0], SIZE_CCODES, sizeof( CONVCODE), cmpcode)) == (CONVCODE *) NULL) {
			if(XPG_flag[cd]) {
				/* for XPG4 compliance, move same char back
				 * or return error (see XLT_1 and XLT_2) */
				lookup = XPG_default;
			} else
				lookup = &defch;
		}
		table_ptr[i] = lookup->char_code;
	}

	/* close conversion tables and leave */
	if (close( fd1) < 0) {
		return BAD;
	}
	if (close( fd2) < 0) {
		return BAD;
	}
	
	return GOOD;
}

/*
**************************************************************************
** Return size of table if conversion code table exists.
** Otherwise, return BAD.
**************************************************************************
*/

static int
ccodes( tocode, fromcode)
char *tocode;			/* "to" code set name */
char *fromcode;			/* "from" code set name */
{
	struct stat buf;	/* buffer for stat call */

	/* get "to" pathname */
	strcpy( Toname, _Dir);
	strcat( Toname, "/ccodes/");
	strcat( Toname, tocode);

	/* "to" file exists ? */
	if (stat( Toname, &buf) < 0) {
		return BAD;
	}

	/* get "from" pathname */
	strcpy( Fromname, _Dir);
	strcat( Fromname, "/ccodes/");
	strcat( Fromname, fromcode);

	/* "from" file exists ? */
	if (stat( Fromname, &buf) < 0) {
		return BAD;
	}

	return SIZE_CCODES;	/* size of table */
}

/*
**************************************************************************
** Return size of table if direct translation table exists.
** Otherwise, return BAD.
**************************************************************************
*/

static int
is_direct( tocode, fromcode)
char *tocode;			/* "to" code set name */
char *fromcode;			/* "from" code set name */
{
	int len;		/* length of code set name */
	struct stat buf;	/* buffer for stat call */

	/* get xlate table pathname */
	(void) strcpy( Toname, _Dir);
	(void) strcat( Toname, "/direct/");
	(void) strncat( Toname, fromcode, CNAME_LEN);
	if ((len = strlen( fromcode)) > CNAME_LEN) {
		(void) strcat( Toname, fromcode+len-1);
	}
	(void) strcat( Toname, "}");
	(void) strncat( Toname, tocode, CNAME_LEN);
	if ((len = strlen( tocode)) > CNAME_LEN) {
		(void) strcat( Toname, tocode+len-1);
	}

	/* does xlate table exists ? */
	if (stat( Toname, &buf) < 0) {
		return BAD;
	}
	
	/* xlate table size in bytes (without header) */
	return (int) buf.st_size - HEADER_SIZE;
}

static int
direct( tocode, fromcode)
char *tocode;			/* "to" code set name */
char *fromcode;			/* "from" code set name */
{
	register int size;	/* table size */
	register int fd;	/* file descriptor */

	/* does xlate table exists ? */
	if ((size = is_direct( tocode, fromcode)) == BAD) {
		return BAD;
	}

	/* open the xlate table */
	if ((fd = open( Toname, O_RDONLY)) < 0) {
		return BAD;
	}

	/* read in the header */
	if (read_header( fd) < 0) {
		return BAD;
	}

	/* need some more space for these 2-byte tables */
	if (Fromindex == HP15 || Fromindex == EBCDIC16) {
		size += Fof2_max * sizeof( CH2 *);
	}

	/* don't need need a table for these conversions */
	if (Fromindex >= ALGO && Toindex >= ALGO) {
		size = 0;
	}

	/* close xlate table */
	if (close( fd) < 0) {
		return BAD;
	}

	/* return size needed for table */
	return size;
}

/*
**************************************************************************
** Return size of table in bytes if a translation table is neeeded.
** Return 0 if a translation table is not neeeded.
** Return BAD if a translation table is neeeded but does not exist.
**************************************************************************
*/

#ifdef _NAMESPACE_CLEAN
#undef iconvsize
#pragma _HP_SECONDARY_DEF _iconvsize iconvsize
#define iconvsize _iconvsize
#endif /* _NAMESPACE_CLEAN */

int
iconvsize( tocode, fromcode)
char *tocode;			/* "to" code set name */
char *fromcode;			/* "from" code set name */
{
	int size;

#ifdef _THREAD_SAFE
        _rec_mutex_lock(&_iconv_rmutex);
#endif
	if ((size = direct( tocode, fromcode)) == BAD  &&
   	    (size = ccodes( tocode, fromcode)) == BAD) {
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return BAD;
	}
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
	return size;
}

/*
**************************************************************************
** Do all initializations needed to do a conversion.
** Return a conversion desciptor if everything is set up ok.
** Otherwise, return BAD.
** A 2-dimensional jump table indexed by [fromcode][tocode] is used to
** initialize conversion routines.
**************************************************************************
*/

#ifdef _NAMESPACE_CLEAN
#undef iconvopen
#pragma _HP_SECONDARY_DEF _iconvopen iconvopen
#define iconvopen _iconvopen
#endif /* _NAMESPACE_CLEAN */

iconvd
iconvopen( tocode, fromcode, table_ptr, d1, d2)
char *tocode;				/* "to" code set name */
char *fromcode;				/* "from" code set name */
UCHAR *table_ptr;			/* where to put xlate table */
int d1;					/* default single-byte character */
int d2;					/* default multi-byte character */
{
	/* initialization routine jump table */

	static PFI init[] = {

		/* Table driven conversion routines */

/* 0: dir */	no_op,
/* 1: ccd */	no_op,
/* 2: hp15 */	init_hp15,
/* 3: bcd */	init_bcd,
/* 4: jis */	init_jis,
/* 5: euc */	init_euc,

		/* Algoritmic conversion routines */

/* 6: jis */	init_jis,
/* 7: ujis */	no_op,
/* 8: sjis */	init_sjis,
	};

	/* ICONV routine jump table */

	static PFI convert[][sizeof( init) / sizeof( PFI)] = {

		/* Table driven conversion routines */

		/* 0     1     2     3     4     5     6     7     8  */ 
/* 0: dir */	{one_one,no_op,no_op,no_op,no_op,no_op,no_op,no_op,no_op},
/* 1: ccd */	{no_op,one_one,no_op,no_op,no_op,no_op,no_op,no_op,no_op},
/* 2: hp15 */	{no_op,no_op,hp15_hp15,hp15_bcd,no_op,no_op,no_op,no_op,no_op},
/* 3: bcd */	{no_op,no_op,bcd_hp15,no_op,no_op,no_op,no_op,no_op,no_op},
/* 4: jis */	{no_op,no_op,no_op,no_op,no_op,no_op,no_op,no_op,no_op},
/* 5: euc */	{no_op,no_op,no_op,no_op,no_op,no_op,no_op,no_op,no_op},

		/* Algoritmic conversion routines */

		/* 0     1     2     3     4     5     6     7     8  */ 
/* 6: jis */	{no_op,no_op,no_op,no_op,no_op,no_op,no_op,jis_ujis,jis_sjis},
/* 7: ujis */	{no_op,no_op,no_op,no_op,no_op,no_op,ujis_jis,no_op,ujis_sjis},
/* 8: sjis */	{no_op,no_op,no_op,no_op,no_op,no_op,sjis_jis,sjis_ujis,no_op},
	};

	register iconvd cd;	/* conversion descriptor */

#ifdef _THREAD_SAFE
        _rec_mutex_lock(&_iconv_rmutex);
#endif
	/* get conversion descriptor */
	for (cd=0; cd < MAX_CD ; cd++) {
		if (! Conv_descriptor[cd]) {
			Conv_descriptor[cd] = 1;
			break;
		}
	}
	if (cd == MAX_CD) {
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return (iconvd) BAD;
	}

	/* get default single-byte character */
	if (d1 <= 255) {
		Default1[cd] = d1 & 0xff;
	}
	else {
		Conv_descriptor[cd] = 0;
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return BAD;
	}

	/* get default multi-byte character */
	if (d2 <= _64K) {
		Default2[cd][0] = (d2 >> 8) & 0xff;
		Default2[cd][1] = d2 & 0xff;
	}
	else {
		Conv_descriptor[cd] = 0;
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return BAD;
	}

	if (table_ptr) { 
		/* table based conversion */

		/* be sure there is a conversion table  */
		PFI routine;
		if (is_direct( tocode, fromcode) != BAD) {
			routine = read_direct;
		}
		else if (ccodes( tocode, fromcode) != BAD) {
			routine = read_ccodes;
		}
		else {
			Conv_descriptor[cd] = 0;
#ifdef _THREAD_SAFE
			_rec_mutex_unlock(&_iconv_rmutex);
#endif
			return (iconvd) BAD;
		}

		/* read in conversion table */
		if ((*routine)( cd, table_ptr) == BAD) {
			Conv_descriptor[cd] = 0;
#ifdef _THREAD_SAFE
			_rec_mutex_unlock(&_iconv_rmutex);
#endif
			return (iconvd) BAD;
		}
	}
	else {
		/* algorithmic based conversion */
#ifdef _THREAD_SAFE
                /*
                 * The following is done to set Fromindex and Toindex.  It is
                 * necessary because of threads.
                 * There is an assumption being made that the user has
                 * called iconvsize immediately before this (probably not
                 * a good assumption even without threads), thus causing
                 * Fromindex and Toindex to be set.  In a threaded environment,
                 * we cannot make this assumption.  Note that if conversion
                 * tables are being used then table_ptr should not be NULL.
                 */
                int fd;
                if (is_direct(tocode, fromcode) == BAD)
                        Fromindex = -1;
                else if ((fd = open(Toname, O_RDONLY)) == -1)
                        Fromindex = -1;
                else {
                        if (read_header(fd) < 0)
                                Fromindex = -1;
                        close(fd);
                }
#endif

		/* be sure we don't need a conversion table  */
		if (Fromindex < ALGO || Toindex < ALGO) {
			Conv_descriptor[cd] = 0;
#ifdef _THREAD_SAFE
			_rec_mutex_unlock(&_iconv_rmutex);
#endif
			return (iconvd) BAD;
		}
	}

	/* be sure there is a good "from" and "to" jump table index */
	if (! (0 <= Fromindex && Fromindex <= (sizeof(init)/sizeof(PFI))-1) ) {
		Conv_descriptor[cd] = 0;
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return BAD;
	}
	if (! (0 <= Toindex && Toindex <= (sizeof(init)/sizeof(PFI))-1) ) {
		Conv_descriptor[cd] = 0;
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return BAD;
	}

	/* initialize "from" routine */
	if ((*init[Fromindex])( cd, FROM) == BAD) {
		Conv_descriptor[cd] = 0;
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return BAD;
	}

	/* initialize "to" routine */
	if ((*init[Toindex])( cd, TO) == BAD) {
		Conv_descriptor[cd] = 0;
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return BAD;
	}

	/* initialize _iconv function */
	if ((__iconv[cd] = convert[Fromindex][Toindex]) == no_op) {
		Conv_descriptor[cd] = 0;
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return BAD;
	}

	/* initialize _iconv1 function */
	/* for now, only two routines */
	if (Fromindex == DIRECT8 && Toindex == DIRECT8) {
		/* single-byte code sets */
		__iconv1[cd] = ind_one_one;
	}
	else {
		/* multi-byte code sets */
		__iconv1[cd] = ind_one_two;
	}

	/* initialize _iconv2 function */
	/* for now, only one routine */
	__iconv2[cd] = ind_two_two;

#ifdef _THREAD_SAFE
	_rec_mutex_unlock(&_iconv_rmutex);
#endif
	/* return converstion descriptor */
	return cd;
}

/*
**************************************************************************
** Return GOOD if conversion descriptor placed by on ready list successfully.
** Otherwise, return BAD.
**************************************************************************
*/

#ifdef _NAMESPACE_CLEAN
#undef iconvclose
#pragma _HP_SECONDARY_DEF _iconvclose iconvclose
#define iconvclose _iconvclose
#endif /* _NAMESPACE_CLEAN */

int
iconvclose( cd)
iconvd cd;		/* conversion descriptor to close */
{
#ifdef _THREAD_SAFE
        _rec_mutex_lock(&_iconv_rmutex);
#endif
	if ( (cd >= 0) && (cd < MAX_CD) && (Conv_descriptor[cd] == 1) ) {
		Conv_descriptor[cd] = 0;
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return GOOD;
	}
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
	return BAD;
}

/*
**************************************************************************
** Copy lock-shift string into cs_in or cs_out structures.
** Return GOOD if everything is ok, otherwise BAD.
**************************************************************************
*/

#ifdef _NAMESPACE_CLEAN
#undef iconvlock
#pragma _HP_SECONDARY_DEF _iconvlock iconvlock
#define iconvlock _iconvlock
#endif /* _NAMESPACE_CLEAN */

int
iconvlock( cd, direction, lock, s)
iconvd cd;				/* conversion descriptor */
int direction;				/* input or output string ? */
int lock;				/* which lock shift string ? */
char *s;				/* the lock shift string */
{
	iconvinfo *cs;			/* ptr to code set structure */
	iconvele *cs_ele;		/* ptr to code set element */

#ifdef _THREAD_SAFE
        _rec_mutex_lock(&_iconv_rmutex);
#endif
	if (direction == FROM ) {
		cs = &cs_in[cd];	/* ptr to input shift & state info */
	}
	else if (direction == TO ) {
		cs = &cs_out[cd];	/* ptr to output shift & state info */
	}
	else {
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return BAD;
	}

	cs->state = START;		/* initialize state */

	if (lock == 0) {
		cs_ele = &(cs->LS0);	/* ptr to lock shift 0 element */
	}
	else if (lock == 1) {
		cs_ele = &(cs->LS1);	/* ptr to lock shift 1 element */
	}
	else if (lock == 2) {
		cs_ele = &(cs->LS2);	/* ptr to lock shift 2 element */
	}
	else {
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return BAD;
	}

	/* find length of shift string, quit if too long */
	if ((cs_ele->len = strlen( s)) > MAX_LOCK) {
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return BAD;
	}

	/* copy shift string */
	(void) strcpy( cs_ele->str, s);

#ifdef _THREAD_SAFE
	_rec_mutex_unlock(&_iconv_rmutex);
#endif
	return GOOD;
}

/*
**************************************************************************
** Take a Shift-Jis char and convert it into a Jis char.
** Return pointer to Jis char.
**************************************************************************
*/

static char *
csjtojis( s1, s2)
char *s1;			/* ptr to jis char */
char *s2;			/* ptr to shift-jis char */
{
	extern void tojis();

	tojis( (UCHAR *) s2, (UCHAR *) s1);
	return s1;
}

/*
**************************************************************************
** Take a Jis char and convert it into a Shift-Jis char.
** Return pointer to Shift-Jis char.
**************************************************************************
*/

static char *
cjistosj( s1, s2)
char *s1;			/* ptr to shift-jis char */
char *s2;			/* ptr to jis char */
{
	extern void tosjis();

	tosjis( (UCHAR *) s2, (UCHAR *) s1);
	return s1;
}

/*
**************************************************************************
** Take a Shift-Jis char and convert it into a Ujis char.
** Return pointer to Ujis char.
**************************************************************************
*/

static char *
csjtouj( s1, s2)
char *s1;			/* ptr to Ujis char */
char *s2;			/* ptr to Shift-Jis char */
{
	extern void tojis();

	tojis( (UCHAR *) s2, (UCHAR *) s1);
	s1[0] |= 0x80;
	s1[1] |= 0x80;
	return s1;
}

/*
**************************************************************************
** Take a Ujis char and convert it into a Shift-Jis char.
** Return pointer to Shift-Jis char.
**************************************************************************
*/

static char *
cujtosj( s1, s2)
char *s1;			/* ptr to shift-jis char */
char *s2;			/* ptr to ujis char */
{
	extern void tosjis();
	char buf[2];

	buf[0] = s2[0] & 0x7f;
	buf[1] = s2[1] & 0x7f;
	tosjis( (UCHAR *) buf, (UCHAR *) s1);
	return s1;
}

/*
**************************************************************************
** Take a Shift-Jis char and convert it into a Jis char.
**************************************************************************
*/

static void
tojis(from, to)
UCHAR *from, *to;
{
	int	highisodd;

	if (from[1] >= 0x9f) {
		highisodd = 0;
		to[1] = from[1] - 0x7e;
	} else {
		highisodd = 1;
		to[1] = from[1] - 0x1f;
		if (from[1] >= 0x80)
			to[1]--;
	}

	if (from[0] >= 0xe0)
		to[0] = (from[0] - 0xb0) * 2;
	else
		to[0] = (from[0] - 0x70) * 2;

	if (highisodd)
		to[0]--;
}

/*
**************************************************************************
** Take a Jis char and convert it into a Shift-Jis char.
**************************************************************************
*/

#define isodd(n) ((n) & 1)

static void
tosjis(from, to)
UCHAR *from, *to;
{

	if (isodd(from[0])) {
		if ((to[1] = from[1] + 0x1f) >= 0x7f)
			to[1]++;
	} else
		to[1] = from[1] + 0x7e;

	if (from[0] <= 0x5e)
		to[0] = (from[0] + 1) / 2 + 0x70;
	else
		to[0] = (from[0] + 1) / 2 + 0xb0;
}

/*---------------------------------------------------------------------------*/
/* XPG4 compliant routines */

/*
**************************************************************************
** Do all initializations needed to do a conversion.
** Return a conversion desciptor if everything is set up ok.
** Otherwise, return BAD, after setting errno.
** A 2-dimensional jump table indexed by [fromcode][tocode] is used to
** initialize conversion routines.
**************************************************************************
*/

#ifdef _NAMESPACE_CLEAN
#undef iconv_open
#pragma _HP_SECONDARY_DEF _iconv_open iconv_open
#define iconv_open _iconv_open
#endif /* _NAMESPACE_CLEAN */

iconv_t
iconv_open(tocode, fromcode)
char *tocode;				/* "to" code set name */
char *fromcode;				/* "from" code set name */
{
	/* initialization routine jump table */

	static PFI init[] = {

		/* Table driven conversion routines */

/* 0: dir */	no_op,
/* 1: ccd */	no_op,
/* 2: hp15 */	init_hp15,
/* 3: bcd */	init_bcd,
/* 4: jis */	init_jis,
/* 5: euc */	init_euc,

		/* Algoritmic conversion routines */

/* 6: jis */	init_jis,
/* 7: ujis */	no_op,
/* 8: sjis */	init_sjis,
	};

	/* ICONV routine jump table */

	static PFI convert[][sizeof( init) / sizeof( PFI)] = {

		/* Table driven conversion routines */

		/* 0     1     2     3     4     5     6     7     8  */ 
/* 0: dir */	{one_one,no_op,no_op,no_op,no_op,no_op,no_op,no_op,no_op},
/* 1: ccd */	{no_op,one_one,no_op,no_op,no_op,no_op,no_op,no_op,no_op},
/* 2: hp15 */	{no_op,no_op,hp15_hp15,hp15_bcd,no_op,no_op,no_op,no_op,no_op},
/* 3: bcd */	{no_op,no_op,bcd_hp15,no_op,no_op,no_op,no_op,no_op,no_op},
/* 4: jis */	{no_op,no_op,no_op,no_op,no_op,no_op,no_op,no_op,no_op},
/* 5: euc */	{no_op,no_op,no_op,no_op,no_op,no_op,no_op,no_op,no_op},

		/* Algoritmic conversion routines */

		/* 0     1     2     3     4     5     6     7     8  */ 
/* 6: jis */	{no_op,no_op,no_op,no_op,no_op,no_op,no_op,jis_ujis,jis_sjis},
/* 7: ujis */	{no_op,no_op,no_op,no_op,no_op,no_op,ujis_jis,no_op,ujis_sjis},
/* 8: sjis */	{no_op,no_op,no_op,no_op,no_op,no_op,sjis_jis,sjis_ujis,no_op},
	};

	register iconv_t cd;	/* conversion descriptor */
	register unsigned int size;
	UCHAR *table_ptr;

#ifdef _THREAD_SAFE
        _rec_mutex_lock(&_iconv_rmutex);
#endif
	/* get conversion descriptor */
	for (cd=0; cd < OPEN_MAX ; cd++) {
		if (! Conv_descriptor[cd]) {
			Conv_descriptor[cd] = 1;
			break;
		}
	}
	if (cd == OPEN_MAX) {
		errno = EMFILE;		/* OPEN_MAX descs already open */
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return (iconv_t) BAD;
	}

	/* Get the size of conversion table through a call to iconvsize */

	if ((size = direct( tocode, fromcode)) == BAD  &&
   	    (size = ccodes( tocode, fromcode)) == BAD) {
		Conv_descriptor[cd] = 0;
		errno = EINVAL;		/* invalid conversion */
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return (iconv_t) BAD;
	} else if(size == 0) {
		table_ptr = (UCHAR *) NULL;
	} else if((table_ptr = (UCHAR *)malloc(size)) == (UCHAR *) NULL) {
		Conv_descriptor[cd] = 0;
		errno = ENOMEM;
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return (iconv_t) BAD;
	}
	tab_alloc[cd] = table_ptr;	/* save pointer */
	if (table_ptr) { 
		/* table based conversion */

		/* be sure there is a conversion table  */
		PFI routine;
		if (is_direct( tocode, fromcode) != BAD) {
			routine = read_direct;
		}
		else if (ccodes( tocode, fromcode) != BAD) {
			routine = read_ccodes;
		}
		else {
			Conv_descriptor[cd] = 0;
			if(tab_alloc[cd]) free((void *)tab_alloc[cd]);
			errno = EINVAL;
#ifdef _THREAD_SAFE
			_rec_mutex_unlock(&_iconv_rmutex);
#endif
			return (iconv_t) BAD;
		}

		/* read in conversion table */
		if ((*routine)( cd, table_ptr) == BAD) {
			Conv_descriptor[cd] = 0;
			if(tab_alloc[cd]) free((void *)tab_alloc[cd]);
			errno = EINVAL;
#ifdef _THREAD_SAFE
			_rec_mutex_unlock(&_iconv_rmutex);
#endif
			return (iconv_t) BAD;
		}
	}
	else {
		/* algorithmic based conversion */
#ifdef _THREAD_SAFE
                /*
                 * The following is done to set Fromindex and Toindex.  It is
                 * necessary because of threads.
                 * There is an assumption being made that the user has
                 * called iconvsize immediately before this (probably not
                 * a good assumption even without threads), thus causing
                 * Fromindex and Toindex to be set.  In a threaded environment,
                 * we cannot make this assumption.  Note that if conversion
                 * tables are being used then table_ptr should not be NULL.
                 */
                int fd;
                if (is_direct(tocode, fromcode) == BAD)
                        Fromindex = -1;
                else if ((fd = open(Toname, O_RDONLY)) == -1)
                        Fromindex = -1;
                else {
                        if (read_header(fd) < 0)
                                Fromindex = -1;
                        close(fd);
                }
#endif

		/* be sure we don't need a conversion table  */
		if (Fromindex < ALGO || Toindex < ALGO) {
			Conv_descriptor[cd] = 0;
			errno = EINVAL;
#ifdef _THREAD_SAFE
			_rec_mutex_unlock(&_iconv_rmutex);
#endif
			return (iconv_t) BAD;
		}
	}

	/* be sure there is a good "from" and "to" jump table index */
	if (! (0 <= Fromindex && Fromindex <= (sizeof(init)/sizeof(PFI))-1) ) {
		Conv_descriptor[cd] = 0;
		if(tab_alloc[cd]) free((void *)tab_alloc[cd]);
		errno = EINVAL;
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return (iconv_t) BAD;
	}
	if (! (0 <= Toindex && Toindex <= (sizeof(init)/sizeof(PFI))-1) ) {
		Conv_descriptor[cd] = 0;
		if(tab_alloc[cd]) free((void *)tab_alloc[cd]);
		errno = EINVAL;
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return (iconv_t) BAD;
	}

	/* initialize "from" routine */
	if ((*init[Fromindex])( cd, FROM) == BAD) {
		Conv_descriptor[cd] = 0;
		if(tab_alloc[cd]) free((void *)tab_alloc[cd]);
		errno = EINVAL;
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return (iconv_t) BAD;
	}

	/* initialize "to" routine */
	if ((*init[Toindex])( cd, TO) == BAD) {
		Conv_descriptor[cd] = 0;
		if(tab_alloc[cd]) free((void *)tab_alloc[cd]);
		errno = EINVAL;
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return (iconv_t) BAD;
	}

	/* initialize _iconv function */
	if ((__iconv[cd] = convert[Fromindex][Toindex]) == no_op) {
		Conv_descriptor[cd] = 0;
		if(tab_alloc[cd]) free((void *)tab_alloc[cd]);
		errno = EINVAL;
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return (iconv_t) BAD;
	}

	/* set up flag indicating that the calls used are XPG4 calls */
	XPG_flag[cd] = 1;

#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
	/* return converstion descriptor */
	return cd;
}

/*
**************************************************************************
** Return GOOD if conversion descriptor placed by on ready list successfully.
** Otherwise, return BAD, and set errno.
**************************************************************************
*/

#ifdef _NAMESPACE_CLEAN
#undef iconv_close
#pragma _HP_SECONDARY_DEF _iconv_close iconv_close
#define iconv_close _iconv_close
#endif /* _NAMESPACE_CLEAN */

int
iconv_close(cd)
iconv_t cd;		/* conversion descriptor to close */
{
#ifdef _THREAD_SAFE
        _rec_mutex_lock(&_iconv_rmutex);
#endif
	if ( (cd >= 0) && (cd < OPEN_MAX) && (Conv_descriptor[cd] == 1) ) {
		Conv_descriptor[cd] = 0;
		/* free allocated space if any */
		if(tab_alloc[cd]) free((void *)tab_alloc[cd]);
		XPG_flag[cd] = 0;
		cs_out[cd].LS0.len = 0;
#ifdef _THREAD_SAFE
                _rec_mutex_unlock(&_iconv_rmutex);
#endif
		return GOOD;
	}
	errno = EBADF;
#ifdef _THREAD_SAFE
	_rec_mutex_unlock(&_iconv_rmutex);
#endif
	return BAD;
}

#ifdef _NAMESPACE_CLEAN
#undef iconv
#pragma _HP_SECONDARY_DEF _iconv iconv
#define iconv _iconv
#endif /* _NAMESPACE_CLEAN */

size_t
iconv( cd, inchar, inbytesleft, outchar, outbytesleft)
iconv_t cd;				/* conversion descriptor */
UCHAR **inchar;				/* input buffer character */
int *inbytesleft;			/* num bytes left in input buffer */
UCHAR **outchar;			/* output buffer character */
int *outbytesleft;			/* num bytes left in input buffer */
{
register iconvinfo *cs = &cs_out[cd];	/* ptr to shift & state info */
register iconvele *cs0 = &cs_out[cd].LS0;	/* lockshift-in */
register int i;				/* a counter for MOVEOUT */

	if ( (cd < 0) || (cd >= OPEN_MAX) || (Conv_descriptor[cd] != 1) ){
		errno = EBADF;
		return(-1);
	}

	/* for XPG4, if inchar is null or points to null, 
	 * shift out the reset sequence in the output buffer
	 */
	if(!inchar || !*inchar) {
		cs->state = START;
		if ( outchar && *outchar ) {
			if (*outbytesleft >= cs0->len) {
				for (i=0 ; i < cs0->len ; i++) {
					*(*outchar)++ = *((cs0->str)+i);
					(*outbytesleft)--;
				}
			}
			else{
				errno = E2BIG;
				RETURN BAD;
			}
		}
		RETURN 0;
	}

	return( (*__iconv[cd])(cd,inchar,inbytesleft,outchar,outbytesleft) );
}
