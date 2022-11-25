/* @(#) $Revision: 66.1 $ */      

#ifdef EUC

/*LINTLIBRARY*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define _1kanji __1kanji
#define _2kanji __2kanji
#define _sec_tab __sec_tab
#define _status_tab __status_tab
#define secof2 _secof2
#define firstof2 _firstof2
#define byte_status _byte_status
#define c_colwidth _c_colwidth
#endif

#include	<nl_ctype.h>



/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef _sec_tab
#pragma _HP_SECONDARY_DEF __sec_tab _sec_tab
#define _sec_tab __sec_tab
#endif

int _sec_tab [4] = { 0, 0, 2, 3};

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef _status_tab
#pragma _HP_SECONDARY_DEF __status_tab _status_tab
#define _status_tab __status_tab
#endif

int _status_tab [3] [4] = {
{ ONEBYTE, FIRSTOF2, ONEBYTE, FIRSTOF2 },
{ ONEBYTE, FIRSTOF2, ONEBYTE, FIRSTOF2 },
{ ONEBYTE, ONEBYTE, SECOF2,  SECOF2 }
};


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef firstof2
#pragma _HP_SECONDARY_DEF _firstof2 firstof2
#define firstof2 _firstof2
#endif

firstof2(c) int c;
{
	return FIRSTof2(c);
}


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef secof2
#pragma _HP_SECONDARY_DEF _secof2 secof2
#define secof2 _secof2
#endif

secof2(c) int c;
{
	return SECof2(c);
}


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef byte_status
#pragma _HP_SECONDARY_DEF _byte_status byte_status
#define byte_status _byte_status
#endif

byte_status(c, laststatus)
{
	return BYTE_STATUS(c, laststatus);
}


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef c_colwidth
#pragma _HP_SECONDARY_DEF _c_colwidth c_colwidth
#define c_colwidth _c_colwidth
#endif

c_colwidth(c) int c;
{
	return C_COLWIDTH(c);
}

#else /* EUC */


/* @(#) $Revision: 66.1 $ */      

/*LINTLIBRARY*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define _1kanji __1kanji
#define _2kanji __2kanji
#define _status_tab __status_tab
#define secof2 _secof2
#define firstof2 _firstof2
#define byte_status _byte_status
#endif

#include	<nl_ctype.h>



/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef _status_tab
#pragma _HP_SECONDARY_DEF __status_tab _status_tab
#define _status_tab __status_tab
#endif

int _status_tab [3] [4] = {
{ ONEBYTE, ONEBYTE, ONEBYTE, FIRSTOF2 },
{ ONEBYTE, ONEBYTE, ONEBYTE, FIRSTOF2 },
{ ONEBYTE, ONEBYTE, SECOF2,  SECOF2 }
};


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef firstof2
#pragma _HP_SECONDARY_DEF _firstof2 firstof2
#define firstof2 _firstof2
#endif

firstof2(c) int c;
{
	return FIRSTof2(c);
}


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef secof2
#pragma _HP_SECONDARY_DEF _secof2 secof2
#define secof2 _secof2
#endif

secof2(c) int c;
{
	return SECof2(c);
}


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef byte_status
#pragma _HP_SECONDARY_DEF _byte_status byte_status
#define byte_status _byte_status
#endif

byte_status(c, laststatus)
{
	return BYTE_STATUS(c, laststatus);
}

#endif /* EUC */
