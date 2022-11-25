/* @(#) $Revision: 70.4 $ */      
/* LINTLIBRARY */

#include	<limits.h>
#include	<nl_types.h>
#include 	<locale.h>
#include 	<setlocale.h>
#include 	<collate.h>
#include 	<wchar.h>		/* Needed for _nl_mb_* vars, below */
#include	<wpi.h>                 /* Needed for _nl_mb_* types, below */


int	__nl_langid[N_CATEGORY]	= 0;
int _nl_radix	= '.';			/* radix character */
int _sh_low     = 0;			/* lowest char of shift table domain */
int _sh_high	= 0377;			/* highest char of shift table domain */
int __nl_char_size	= 1;		/* 1 byte char size */
#ifdef EUC
int __nl_code_scheme	= 0;		/* 1 byte code scheme */
int __cs_SBYTE		= 1;		/* single byte code scheme */
int __cs_HP15		= 0;		/* not HP15 code scheme */
int __cs_EUC		= 0;		/* not EUC code scheme */
#endif /* EUC */
nl_direct _nl_direct	= NL_LTR;	/* left-to-right text direction */
int _nl_context = 0;

unsigned char	*_nl_dgt_alt	= (unsigned char *)"";
					/* no alternative digits */

unsigned char	*_nl_dascii	= (unsigned char *)"0123456789 +-.,eE";
					/* ascii digits */

unsigned char 	*_nl_pascii	= (unsigned char *)" !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
					/* ascii punctuation */

unsigned char	*_nl_punct_alt	= (unsigned char *)"";
					/* no alternative punctuations */

#ifdef EUC
unsigned char	__in_csize[] = { 1, 0, 0, 0 };
					/* default input char size */
unsigned char	__out_csize[] = { 1, 0, 0, 0 };
					/* default output char size */

unsigned int __euc_template[] = { 0x00000000, 0x00008080, 0x00000080, 0x00008000 };
					/* euc process code template */
#endif /* EUC */
			
nl_mode	_nl_mode	= NL_NONLATIN;	/* non-Latin mode */
nl_order _nl_order	= NL_KEY;	/* keyboard order */
nl_outdgt _nl_outdigit  = NL_ASCII;	/* ascii digits on output */

struct _era_data	*_nl_era[MAX_ERA_FMTS];	/* era info table */

int	_nl_space_alt	= 32;		/* ascii value for space */
int	_nl_map21	= 0;		/* non-zero if 2-to-1 mappings */
int	_nl_onlyseq	= 1;		/* default no complex mappings */
int 	_nl_collate_on  = 0;		/* default no NLS collation table */
int 	_nl_mb_collate  = 0;		/* default no multibyte collation */


unsigned char	 *_seqtab	= 0;	/* dictionary sequence number table */
unsigned char	 *_pritab	= 0;	/* 1to2/2to1 flag + priority table */
struct col_12tab *_tab12	= 0;	/* 2-to-1 mapping table */
struct col_21tab *_tab21	= 0;	/* 1-to-2 mapping table */

unsigned char _sort_rules[COLL_WEIGHTS_MAX];	/* for collation */

/*  Following are for the isw*() and tow*() macros/functions that do
 *  extended classification/conversion for WPI routines:
 */
_nl_mb_hdr_t	*_nl_mb_hdr = 0;	/* pointer to extended table header */
_nl_mb_data_t	*_nl_mb_cls = 0;	/* pointer to classification table */
_nl_mb_data_t	*_nl_mb_toL = 0;	/* pointer to upper->lower conv table */
_nl_mb_data_t	*_nl_mb_toU = 0;	/* pointer to lower->upper conv table */
wint_t		_nl_wc = 0;		/* used in isw*() macros (<wchar.h>) */
wint_t		_nl_cwc = 0;		/* used in tow*() macros (<wchar.h>) */

