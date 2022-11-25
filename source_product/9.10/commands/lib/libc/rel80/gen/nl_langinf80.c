/* @(#) $Revision: 70.1 $ */      
/*LINTLIBRARY*/
/*
 *	nl_langinfo()
 */
#ifdef _NAMESPACE_CLEAN
#define nl_langinfo _nl_langinfo
#endif

#include	<nl_types.h>
#include	"setlocale.h"

/* nl_langinfo message strings for the "C" locale */
unsigned char	*__C_langinfo[_NL_MAX_MSG+1] = {
	(unsigned char *)"",				/* reserved LC_TIME category */
	(unsigned char *)"%a %b %d %H:%M:%S %Y",	/* D_T_FMT	*/
	(unsigned char *)"%m/%d/%y",			/* D_FMT	*/
	(unsigned char *)"%H:%M:%S",			/* T_FMT	*/
	(unsigned char *)"",				/* reserved LC_TIME category */
	(unsigned char *)"",				/* reserved LC_TIME category */
	(unsigned char *)"Sunday",			/* DAY_1	*/
	(unsigned char *)"Monday",			/* DAY_2	*/
	(unsigned char *)"Tuesday",			/* DAY_3	*/
	(unsigned char *)"Wednesday",			/* DAY_4	*/
	(unsigned char *)"Thursday",			/* DAY_5	*/
	(unsigned char *)"Friday",			/* DAY_6	*/
	(unsigned char *)"Saturday",			/* DAY_7	*/
	(unsigned char *)"Sun",				/* ABDAY_1	*/
	(unsigned char *)"Mon",				/* ABDAY_2	*/
	(unsigned char *)"Tue",				/* ABDAY_3	*/
	(unsigned char *)"Wed",				/* ABDAY_4	*/
	(unsigned char *)"Thu",				/* ABDAY_5	*/
	(unsigned char *)"Fri",				/* ABDAY_6	*/
	(unsigned char *)"Sat",				/* ABDAY_7	*/
	(unsigned char *)"January",			/* MON_1	*/
	(unsigned char *)"February",			/* MON_2	*/
	(unsigned char *)"March",			/* MON_3	*/
	(unsigned char *)"April",			/* MON_4	*/
	(unsigned char *)"May",				/* MON_5	*/
	(unsigned char *)"June",			/* MON_6	*/
	(unsigned char *)"July",			/* MON_7	*/
	(unsigned char *)"August",			/* MON_8	*/
	(unsigned char *)"September",			/* MON_9	*/
	(unsigned char *)"October",			/* MON_10	*/
	(unsigned char *)"November",			/* MON_11	*/
	(unsigned char *)"December",			/* MON_12	*/
	(unsigned char *)"Jan",				/* ABMON_1	*/
	(unsigned char *)"Feb",				/* ABMON_2	*/
	(unsigned char *)"Mar",				/* ABMON_3	*/
	(unsigned char *)"Apr",				/* ABMON_4	*/
	(unsigned char *)"May",				/* ABMON_5	*/
	(unsigned char *)"Jun",				/* ABMON_6	*/
	(unsigned char *)"Jul",				/* ABMON_7	*/
	(unsigned char *)"Aug",				/* ABMON_8	*/
	(unsigned char *)"Sep",				/* ABMON_9	*/
	(unsigned char *)"Oct",				/* ABMON_10	*/
	(unsigned char *)"Nov",				/* ABMON_11	*/
	(unsigned char *)"Dec",				/* ABMON_12	*/
	(unsigned char *)".",				/* RADIXCHAR	*/
	(unsigned char *)"",				/* THOUSEP	*/
	(unsigned char *)"yes",				/* YESSTR	*/
	(unsigned char *)"no",				/* NOSTR	*/
	(unsigned char *)"",				/* CRNCYSTR	*/
	(unsigned char *)"1",				/* BYTES_CHAR	*/
	(unsigned char *)"",				/* DIRECTION	*/
	(unsigned char *)"",				/* ALT_DIGIT	*/
	(unsigned char *)"",				/* ALT_PUNCT	*/
	(unsigned char *)"AM",				/* AM_STR	*/
	(unsigned char *)"PM",				/* PM_STR	*/
	(unsigned char *)"",				/* YEAR_UNIT	*/
	(unsigned char *)"",				/* MON_UNIT	*/
	(unsigned char *)"",				/* DAY_UNIT	*/
	(unsigned char *)"",				/* HOUR_UNIT	*/
	(unsigned char *)"",				/* MIN_UNIT	*/
	(unsigned char *)"",				/* SEC_UNIT	*/
	(unsigned char *)""				/* ERA_FMT	*/
	};

	/*
	 * Note all defined nl_langinfo items must be represented in the above
	 * array even though they may be empty strings.  Also, _NL_MAX_MSG must
	 * be set to the last item number (in setlocale.h).
	 */


unsigned char	**__nl_info = __C_langinfo;				/* use "C" strings as default */

#ifdef _NAMESPACE_CLEAN
#undef nl_langinfo
#pragma _HP_SECONDARY_DEF _nl_langinfo nl_langinfo
#define nl_langinfo _nl_langinfo
#endif
char *nl_langinfo( item )
nl_item item;
{
	if (item > (nl_item)0 && item <= (nl_item)_NL_MAX_MSG)
		return((char *)__nl_info[item]);
	else
		return((char *)"");
}
