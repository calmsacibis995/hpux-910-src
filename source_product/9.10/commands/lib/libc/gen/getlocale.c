/* @(#) $Revision: 70.2 $ */
#pragma HP_SHLIB_VERSION "4/92"
#ifdef _NAMESPACE_CLEAN
#define getlocale _getlocale
#define strcpy    _strcpy
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <locale.h>

extern unsigned char __error_store[N_CATEGORY][SL_NAME_SIZE+1];
extern unsigned char __langmap[N_CATEGORY][LC_NAME_SIZE];
extern unsigned char __modmap[N_CATEGORY][MOD_NAME_SIZE+1];


struct locale_data __ld;

/******************************************************************/
/* getlocale will return locale, modifier or error data.          */
/* The data is copied into the local locale_data structure and    */
/* a pointer to it is returned.  Therefore each call to getlocale */
/* will overwrite the previous call.				  */
/******************************************************************/

#ifdef _NAMESPACE_CLEAN
#undef getlocale
#pragma _HP_SECONDARY_DEF _getlocale getlocale
#define getlocale _getlocale
#endif /* _NAMESPACE_CLEAN */

struct locale_data *
getlocale(type)
int type;
{


	switch(type) {

	case LOCALE_STATUS:
		(void) strcpy(__ld.LC_ALL_D,__langmap[LC_ALL]);
		(void) strcpy(__ld.LC_COLLATE_D,__langmap[LC_COLLATE]);
		(void) strcpy(__ld.LC_CTYPE_D,__langmap[LC_CTYPE]);
		(void) strcpy(__ld.LC_MESSAGES_D,__langmap[LC_MESSAGES]);
		(void) strcpy(__ld.LC_MONETARY_D,__langmap[LC_MONETARY]);
		(void) strcpy(__ld.LC_NUMERIC_D,__langmap[LC_NUMERIC]);
		(void) strcpy(__ld.LC_TIME_D,__langmap[LC_TIME]);
		break;

	case MODIFIER_STATUS:
		(void) strcpy(__ld.LC_ALL_D,__modmap[LC_ALL]);
		(void) strcpy(__ld.LC_COLLATE_D,__modmap[LC_COLLATE]);
		(void) strcpy(__ld.LC_CTYPE_D,__modmap[LC_CTYPE]);
		(void) strcpy(__ld.LC_MESSAGES_D,__modmap[LC_MESSAGES]);
		(void) strcpy(__ld.LC_MONETARY_D,__modmap[LC_MONETARY]);
		(void) strcpy(__ld.LC_NUMERIC_D,__modmap[LC_NUMERIC]);
		(void) strcpy(__ld.LC_TIME_D,__modmap[LC_TIME]);
		break;

	case ERROR_STATUS:
		(void) strcpy(__ld.LC_ALL_D,__error_store[LC_ALL]);
		(void) strcpy(__ld.LC_COLLATE_D,__error_store[LC_COLLATE]);
		(void) strcpy(__ld.LC_CTYPE_D,__error_store[LC_CTYPE]);
		(void) strcpy(__ld.LC_MESSAGES_D,__error_store[LC_MESSAGES]);
		(void) strcpy(__ld.LC_MONETARY_D,__error_store[LC_MONETARY]);
		(void) strcpy(__ld.LC_NUMERIC_D,__error_store[LC_NUMERIC]);
		(void) strcpy(__ld.LC_TIME_D,__error_store[LC_TIME]);
		break;
	}

	return(&__ld);

}
