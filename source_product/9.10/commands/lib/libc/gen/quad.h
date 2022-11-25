/************************************************************************/
/*									*/
/*	HEADER FILE:	quad.h		( V1.0 )	/8/20/88   XM	*/
/*									*/
/*	PURPOSE:	The header file for elementary math package	*/
/*			of quad precision floating point number		*/
/*									*/
/*	DEVELOPED BY:   The Software Development Center( QHSOFT )	*/
/*			& Department of Applied Mathematics,		*/
/*			Tsinghua University, Beijing			*/
/*									*/
/*      MODIFIED BY:    Chih-Ming Hu to include typedef quadfl and      */
/*       		qnumber and to rename UNDERFLOW & OVERFLOW 	*/
/*			so that there is no naming conflict betwenn	*/
/*			this file and /usr/include/math.h, Nov. 20, 88  */
/*									*/
/*			Liz Sanville to add language flag defines.	*/
/*			December 1988.					*/
/************************************************************************/

/***	Quad precision constants				      ***/

#define  SIZE                 4   /*  number of words of the quad type	*/
#define  SIZEPLUS             5	  /*  SIZEPLUS = SIZE + 1		*/
#define  SIZEMINUS            3	  /*  SIZEMINUS = SIZE - 1		*/
#define  LENGTH              32	  /*  word length			*/
#define  P                  113	  /*  precision				*/
#define  BUFSIZE             40	  /*  size of the input buffer		*/ 
#define  BUFSIZEMINUS        39	  /*  BUFSIZEMINUS = BUFSIZE - 1	*/
#define  Emin          (-16382)	  /*  minimum exponent			*/
#define  Emax             16383	  /*  maximum exponent			*/
#define  BIAS             16383	  /*  exponent bias			*/
#define  BIAS_WRAP        24576   /*  the bias adjustment for the wrap	*/ 
#define  INFINITY_EXP     16384   /*  INFINITY_EXP = Emax + 1		*/
#define  WIDTH              128	  /*  format width in bits		*/
#define  NSR                 19	  /*  number of the subregion		*/
#define  NSRMINUS            18	  /*  NSRMINUS = NSR - 1		*/
#define  HOF                 17	  /*  high order of the fraction	*/
#define  EXPW                15	  /*  exponent width in bits		*/
#define  EXPWPLUS            16	  /*  EXPWPLUS = EXPW - 1		*/

/***	Exceptions						      ***/

#define  NOEXCEPTION	   0x00
#define  UNIMPLEMENT       0x01
#define  INEXACT           0x02
#define  IUNDERFLOW        0x04
#define  IOVERFLOW         0x08
#define  DIVIDEBYZERO      0x10
#define  INVALIDOPERATION  0x20

/***	Useful typedefs						      ***/

typedef  unsigned int      quad[SIZE];
typedef  unsigned int      QUIN[SIZEPLUS];
typedef  unsigned int	   QUAD[SIZE+SIZE];
typedef  unsigned int	   DBL_INTEGER[2];
typedef  unsigned int      QUAD_INTEGER[SIZE];

typedef struct {
	unsigned int w0;
	unsigned int w1;
	unsigned int w2;
	unsigned int w3;
} quadfl;

typedef union {
	quadfl flval;
	quad   ptval;
} qnumber;

/***	Print array						      ***/

#define  W(z) printf("z=%x %x %x %x %x\n",z[0],z[1],z[2],z[3],z[4]);

/***	To add the value of argument d to a number of n+1 words	      ***/

#define INC(w,d,n) { int u, c, c0;					\
		     c = d;						\
		     for( u=n; u>=0; u-- )				\
			{ c0 = w[u];					\
			  w[u] += c;					\
			  if( w[u]<c0 && w[u]<c )  c = 1;		\
			  else break;					\
			}						\
		   }

/***	Shifting all bits of each element of the array( w[0],...,     ***/
/***	w[l-1] ) to the right by n ( 0 <= n <= 31 ) bits	      ***/

#define RSHIFT(w,n,l) { int u;						\
			for( u=l-1; u>0; u-- )				\
			   w[u] = w[u] >> n | w[u-1] << LENGTH - ( n );	\
			w[0] >>= n;					\
		      } 

/***	Shifting all bits of each element of the array( w[0],...,     ***/
/***	w[l-1] ) to the left by n ( 0 <= n <= 31 ) bits		      ***/

#define LSHIFT(w,n,l) { int u;						\
			for( u=0; u<l-1; u++ )				\
			   w[u] = w[u] << n | w[u+1] >> LENGTH - ( n );	\
			w[l-1] <<= n;					\
		      }


/***	Declaration of routines included in the quad package		***/

extern qfsn(),     qfns();
extern qfadd(),    qfsub(),    qfmpy(),    qfdiv(),    qfabs();
extern qfsqrt(),   qfrem(),    qfrnd(),    qfcmp(),    qftest();
extern sqfcnvff(), dqfcnvff(), qfcpy(),    qdfcnvff(), qsfcnvff();
extern sqfcnvfx(), dqfcnvfx(), qqfcnvfx(), qdfcnvfx(), qsfcnvfx();
extern sqfcnvxf(), dqfcnvxf(), qqfcnvxf(), qdfcnvxf(), qsfcnvxf();
extern sqfcnvfxt(),dqfcnvfxt(),qqfcnvfxt(),qdfcnvfxt(),qsfcnvfxt();
extern qxsn(),     qxns();

/***	Declaration of routines included in the externded package	***/

extern efsn(),     efns();
extern efadd(),    efsub(),    efmpy(),    efdiv(),    efabs();
extern efsqrt(),   efrem(),    efrnd(),    efcmp(),    eftest();
extern sefcnvff(), defcnvff(), efcpy(),    edfcnvff(), esfcnvff();
extern sefcnvfx(), defcnvfx(), eefcnvfx(), edfcnvfx(), esfcnvfx();
extern sefcnvxf(), defcnvxf(), eefcnvxf(), edfcnvxf(), esfcnvxf();
extern sefcnvfxt(),defcnvfxt(),eefcnvfxt(),edfcnvfxt(),esfcnvfxt();
extern exsn(),     exns();

#define qqfcnvfx(a, b, c, d)	_U_qqfcnvfx(a, b, c, d)
#define qsfcnvfxt(a, b, c, d)	_U_qsfcnvfxt(a, b, c, d)
#define sqfcnvxf(a, b, c, d)	_U_sqfcnvxf(a, b, c, d)
#define dqfcnvff(a, b, c, d)	_U_dqfcnvff(a, b, c, d)
#define qdfcnvff(a, b, c, d)	_U_qdfcnvff(a, b, c, d)
#define qfsqrt(a, b, c, d)	_U_qfsqrt(a, b, c, d)
#define qfcpy(a, b, c, d)	_U_qfcpy(a, b, c, d)
#define qfmpy(a, b, c, d)	_U_qfmpy(a, b, c, d)
#define qfcmp(a, b, c, d)	_U_qfcmp(a, b, c, d)
#define qfadd(a, b, c, d)	_U_qfadd(a, b, c, d)
#define qfsub(a, b, c, d)	_U_qfsub(a, b, c, d)
#define qfdiv(a, b, c, d)	_U_qfdiv(a, b, c, d)

/* Language flags - added by Liz Sanville 12/88 */
#define ANSIC	0
#define FORTRAN	1
#define PASCAL	2	/* for future use */
#define ADA	3	/* for future use */

/* Extern array declarations for data in qfdata.c 
	- added by Liz Sanville 2/89 */
extern int _e[];
extern QUIN _q0[];
extern QUIN _q1[];
extern unsigned _ls[];
