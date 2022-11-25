/* @(#) $Revision: 64.6 $ */      

/*LINTLIBRARY*/





/*
** The code below marked as needed for backward compatibility is needed
** to support delta 37.1 of <nl_ctype.h> (release 1.0 of hp9000s840).
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define _1kanji __1kanji
#define _2kanji __2kanji
#define _nl_failid __nl_failid
#define langinit _langinit
#define ld_nl_ctype _ld_nl_ctype
#define idtolang _idtolang
#define setlocale _setlocale
#endif

#include	<locale.h>
#include	<setlocale.h>

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef _nl_failid
#pragma _HP_SECONDARY_DEF __nl_failid _nl_failid
#define _nl_failid __nl_failid
#endif

int _nl_failid = -1;	/* initialize failed langid to unused number */

/* vvvvvvvvvvvvvvvvvvvvvv start backward compatibility vvvvvvvvvvvvvvvvvvvvv */
char _nl_ctype[1281];
/* ^^^^^^^^^^^^^^^^^^^^^^  end backward compatibility  ^^^^^^^^^^^^^^^^^^^^^ */


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef langinit
#pragma _HP_SECONDARY_DEF _langinit langinit
#define langinit _langinit
#endif

int langinit(langname)
char *langname;
{

/* vvvvvvvvvvvvvvvvvvvvvv start backward compatibility vvvvvvvvvvvvvvvvvvvvv */
	int i, ret_val;
/* ^^^^^^^^^^^^^^^^^^^^^^  end backward compatibility  ^^^^^^^^^^^^^^^^^^^^^ */

	/*
	** Use setlocale to load the requested language.
	** If setlocale fail reset entire environment to "n-computer"
	** to maintain backward-compatibiltiy.
	*/

	char *locale;

	if (langname==0 || *langname=='\0')
		locale=setlocale(LC_ALL,"n-computer");
	else {
		locale=setlocale(LC_ALL,langname);
		if (locale == 0) 
			(void) setlocale(LC_ALL,"n-computer");
	}
	ret_val = ((locale == 0) ? -1 : 0);


/* vvvvvvvvvvvvvvvvvvvvvv start backward compatibility vvvvvvvvvvvvvvvvvvvvv */
	_nl_ctype[256] = (__ctype-1)[256];
	for (i=0; i<256; i++) {
		_nl_ctype[i]	  = (__ctype-1)[i];
		_nl_ctype[i+641]  = _1kanji[i];
		_nl_ctype[i+1025] = _2kanji[i];
	}
	for (i=128; i<256; i++) {
		_nl_ctype[i-128+513] = _1kanji[i];
		_nl_ctype[i-128+897] = _2kanji[i];
	}
/* ^^^^^^^^^^^^^^^^^^^^^^  end backward compatibility  ^^^^^^^^^^^^^^^^^^^^^ */

	return (ret_val);
}

/* vvvvvvvvvvvvvvvvvvvvvv start backward compatibility vvvvvvvvvvvvvvvvvvvvv */
int _loadlang = -1;
int _faillang = -1;

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef ld_nl_ctype
#pragma _HP_SECONDARY_DEF _ld_nl_ctype ld_nl_ctype
#define ld_nl_ctype _ld_nl_ctype
#endif

ld_nl_ctype(langid)
{
	_loadlang = -1;
	_faillang = langid;

	if ((langid < 0) || (langinit(idtolang(langid))))
		return -1;
	
	_loadlang = langid;
	_faillang = -1;
	return 0;
}
/* ^^^^^^^^^^^^^^^^^^^^^^  end backward compatibility  ^^^^^^^^^^^^^^^^^^^^^ */
