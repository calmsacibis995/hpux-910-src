/* @(#) $Revision: 70.2 $ */
#pragma HP_SHLIB_VERSION "4/92"
#ifdef _NAMESPACE_CLEAN
#define getlocale _getlocale
#define getenv _getenv
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <setlocale.h>
#include <nl_types.h>


/*  Routine to return string describing invalid locale-related  */
/*  environment variables   */

extern struct locale_data *getlocale();
extern char *getenv();

#define set(var,value)	var |= (1<<value)
#define is_set(var,value)  (var & (1<<value))

struct locale_data *__error_locale;

char *_errlocale(s)
unsigned char *s;
{
    register int i;
    int lang_bad_flag =0;
    int found_bad_lang=0;
    int locale_bad_flag =0;
    unsigned char *lang;
    register unsigned char *errbufptr = __errptr; /* pts to buffer from *
                                                   * setlocale ~1.4K    */

    if (*s) {
        errbufptr += x_strcpy(errbufptr,s);
	*errbufptr++ = ':';
	*errbufptr++ = ' ';
    }
    errbufptr += x_strcpy(errbufptr, (unsigned char *)"Warning!  The following language(s) are not available:\n");

    /* check each category for invalid locale. */

    __error_locale=getlocale(ERROR_STATUS);
    if (*__error_locale->LC_ALL_D != '\0')
	set(found_bad_lang,LC_ALL);
    if (*__error_locale->LC_COLLATE_D != '\0')
	set(found_bad_lang,LC_COLLATE);
    if (*__error_locale->LC_CTYPE_D != '\0')
	set(found_bad_lang,LC_CTYPE);
    if (*__error_locale->LC_MESSAGES_D != '\0')
	set(found_bad_lang,LC_MESSAGES);
    if (*__error_locale->LC_MONETARY_D != '\0')
	set(found_bad_lang,LC_MONETARY);
    if (*__error_locale->LC_NUMERIC_D != '\0')
	set(found_bad_lang,LC_NUMERIC);
    if (*__error_locale->LC_TIME_D != '\0')
	set(found_bad_lang,LC_TIME);

    for (i=0; i<N_CATEGORY; i++)
    {
	if (is_set(found_bad_lang,i))
	{
            lang = (unsigned char *) getenv(__category_name[i]);
            if  ((i == LC_ALL) || (lang == NULL) || (*lang == '\0'))
	    {
                if (!lang_bad_flag)  /* Bad lang message not printed yet */
		{
                    errbufptr += x_strcpy(errbufptr, (unsigned char *)"\t\tLANG=");
		    switch (i)  {
		    case LC_ALL:
                    	errbufptr += x_strcpy(errbufptr, __error_locale->LC_ALL_D);
			break;
		    case LC_COLLATE:
                    	errbufptr += x_strcpy(errbufptr, __error_locale->LC_COLLATE_D);
			break;
		    case LC_CTYPE:
                    	errbufptr += x_strcpy(errbufptr, __error_locale->LC_CTYPE_D);
			break;
		    case LC_MESSAGES:
                    	errbufptr += x_strcpy(errbufptr, __error_locale->LC_MESSAGES_D);
			break;
		    case LC_MONETARY:
                    	errbufptr += x_strcpy(errbufptr, __error_locale->LC_MONETARY_D);
			break;
		    case LC_NUMERIC:
                    	errbufptr += x_strcpy(errbufptr, __error_locale->LC_NUMERIC_D);
			break;
		    case LC_TIME:
                    	errbufptr += x_strcpy(errbufptr, __error_locale->LC_TIME_D);
			break;
		    }
		    *errbufptr++ = '\n';
                    lang_bad_flag++;
		}
	    }
	    else  /* environment variable itself has invalid locale. */
	    {
		errbufptr += x_strcpy(errbufptr, (unsigned char *)"\t\t");
		errbufptr += x_strcpy(errbufptr, __category_name[i]);
		*errbufptr++ = '=';
		switch (i)  {
		    case LC_ALL:
                    	errbufptr += x_strcpy(errbufptr, __error_locale->LC_ALL_D);
			break;
		    case LC_COLLATE:
                    	errbufptr += x_strcpy(errbufptr, __error_locale->LC_COLLATE_D);
			break;
		    case LC_CTYPE:
                    	errbufptr += x_strcpy(errbufptr, __error_locale->LC_CTYPE_D);
			break;
		    case LC_MESSAGES:
                    	errbufptr += x_strcpy(errbufptr, __error_locale->LC_MESSAGES_D);
			break;
		    case LC_MONETARY:
                    	errbufptr += x_strcpy(errbufptr, __error_locale->LC_MONETARY_D);
			break;
		    case LC_NUMERIC:
                    	errbufptr += x_strcpy(errbufptr, __error_locale->LC_NUMERIC_D);
			break;
		    case LC_TIME:
                    	errbufptr += x_strcpy(errbufptr, __error_locale->LC_TIME_D);
			break;
		}
		*errbufptr++ = '\n';
                locale_bad_flag++;
	    }
	}
    }
    (void) x_strcpy(errbufptr, (unsigned char *)"Continuing processing using the language \"C\".\n");
    
    if (lang_bad_flag || locale_bad_flag)
        return((char *)__errptr);
    else
        return(NULL);
}



static x_strcpy(to, from)
register unsigned char *to, *from;
{
	register unsigned char *base = to;

	while (*from)
		*to++ = *from++;
	
	*to = '\0';

	return (to - base);
}
