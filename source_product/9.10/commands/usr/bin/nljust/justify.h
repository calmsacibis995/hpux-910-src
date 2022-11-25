/* @(#) $Revision: 51.1 $ */   

/*
**************************************************************************
** Constants
**************************************************************************
*/

/* return codes */

#define SUCCESSFUL 	0
#define UNSUCCESSFUL	1

/* boolean values */

#define TRUE 		1
#define FALSE 		0

/* control characters to select right-to-left character set */

#define SO	 	'\016'
#define SI	 	'\017'

/* some more characters */

#define EOL	 	'\000'
#define NEW_LINE 	'\012'
#define ESC	 	'\033'
#define SPACE 		'\040'
#define NL_SPACE 	'\240'

/* line length */

#define MAX_LINE	512
#define FILL		4

/* default maximum printer width */

#define MAX_WIDTH	80

/* character masks */

#define HI_BIT 		0x80
#define NL_MASK		0x80
#define L_MASK		0x00

/* message numbers */

#define BAD_LANG	1	/* LANG not equal to a right-to-left language */
#define BAD_LEFT	2	/* non-latin file with left justification */
#define BAD_RIGHT	3	/* latin mode file with right justification */
#define NO_MEMORY	4	/* out of memory */
#define IN_OVERFLOW	5	/* input buffer overflow */
#define OUT_OVERFLOW	6	/* output buffer overflow */
#define BAD_1WIDTH	7	/* width exceeds input buffer length */
#define BAD_2WIDTH	8	/* invalid printer width */
#define BAD_1MAR	9	/* margin exceeds input buffer length */
#define BAD_2MAR	10	/* invalid wrap margin */
#define BAD_3MAR	11	/* margin exceeds printer width (part 1) */
#define BAD_1TAB	12	/* tab stop exceeds input buffer length */
#define BAD_2TAB	13	/* invalid tab stop number */
#define BAD_INPUT	14	/* unable to open input file */
#define BAD_JUST	15	/* invalid justification (l or r) */
#define BAD_MODE	16	/* invalid mode (l or n) */
#define BAD_ORDER	17	/* invalid order (k or s) */
#define BAD_USAGE	18	/* usage error */

/* severity of message */

#define FATAL		1
#define WARN		0

/*
**************************************************************************
** Typedefs
**************************************************************************
*/

typedef enum {RIGHT,LEFT}		JUST;	/* type of justification */
typedef enum {WRAP,TRUNC}		END;	/* type of word wrap */
typedef enum {ARABIC,HEBREW,OTHER}	LANG;	/* type of right-to-left lang */
typedef unsigned char			UCHAR;	/* unsigned character type */

typedef struct {
	int stop;	/* number of chars between tab stops: default 8 */
	UCHAR tab;	/* tab character: default tab (0x09) */
} TAB;

/*
**************************************************************************
** Macros
**************************************************************************
*/

/* switch pointers to input and output buffers */

#define SWITCH(one,two,temp)	temp=one ; one=two ; two=temp 

/* isspace macro counting alternative space */

#define _ISSPACE(p)		( isspace((int)p) || (p) == AltSpace )

/* test most significant bit of character (non-latin msb set) */

#define NL_CHAR(p)		( (p) & HI_BIT )

/* store value into the contents of pointer if room */

#define STORE(p,value,len)	if (len < MAX_LINE) {			\
					*p = value;			\
				} else {				\
					put_message(FATAL,OUT_OVERFLOW);	\
				}

/* test for opposite language character */

#define OPP_LANG(c)		(( (c) & HI_BIT ) ^ Mask)
