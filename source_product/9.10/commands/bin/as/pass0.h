/* @(#) $Revision: 70.1 $ */      

#define MAXTMPF	8

#define TMPDIR		"/tmp"
#define TMPFILE0	"as"
#define TMPFILE1	"astx"
#define TMPFILE2	"asda"
#define TMPFILE3	"asgn"  /* debug - gntt */
#define TMPFILE4	"assl"
#define TMPFILE5	"asvt" 
#define TMPFILE6	"astr"  
#define TMPFILE7	"asdr"
#define TMPFILE8	"asls"
#define TMPFILE9	"asln"  /* debug - lntt */
#define TMPFILE10	"asxt"  /* debug - xt */
#define TMPFILE11	"asmo"  /* module info */

# define M4		"/usr/bin/m4"

#if BFA
# define EXIT(n)	bfa_exit(n)
# define BFADIR		"./bfa-results"
# define BFAPREFIX	"bo.all"
# else
# define EXIT(n)	exit(n)
# endif
