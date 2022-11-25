/* @(#) $Revision: 66.1 $ */
/* LINTLIBRARY */

#include	<stdio.h>
#include	<limits.h>
#include	"global.h"

/* era_init: initialize era table.
** Get here when you see a 'era' keyword.
*/
void
era_init(token)
int	token;				/* keyword token */
{
	extern void era_str();
	extern void era_finish();

	if (op) {
		/* if we're not the first, finish up last keyword */
		if (finish == NULL) error(STATE);
		(*finish)();
	}

	op = token;			/* save off the token */
	string = era_str;		/* set string function for info.c */
	number = NULL;			/* no numbers allowed */
	finish = era_finish;		/* set up info finish */
}

/* era_str: process one era-information string.
** Remove quote characters, and check length and format of the era string
*/
void
era_str()
{
	extern char *strcpy(),
		    *malloc();
	extern void getstr(),
		    store_era();
	unsigned char buf[LEN_INFO_MSGS];
	struct _era_data *sp;

	(void) getstr(buf);			/* process era string */
	if (strlen(buf) > LEN_INFO_MSGS-1)	/* check length */
		error(ERA_LEN);
	if ((sp = (struct _era_data *)malloc(sizeof(struct _era_data))) == NULL)
		error(NOMEM);
	else {					/* store era structure */
		if (era_num >= MAX_ERA_FMTS)
			error(TOO_MANY_ERA);
		else {
			(void) store_era(sp, buf);	/* store era info */

			era_tab2[2*era_num] = sp->name;			/* save pointer to name */
			sp->name = (unsigned char *)era_len2;		/* convert name ptr to offset */
			era_len2 += strlen(era_tab2[2*era_num]) + 1;

			era_tab2[2*era_num+1] = sp->format;		/* ditto for format */
			sp->format = (unsigned char *)era_len2;
			era_len2 += strlen(era_tab2[2*era_num+1]) + 1;

			era_tab[era_num++] = sp;			/* save the era structure */
			era_len += sizeof(struct _era_data);
		}
	}
}

/* era_finish: finish up any left-overs from last keyword.
** Called when next keyword or end of file appears.
** Make sure that there are no left-over metachars.
*/
void
era_finish()
{
	if (META) error(EXPR);

	era_tab[era_num] = (struct _era_data *)NULL;
}

/* store_era: take era string and store the info into _era_data structure.
** Examine era string in buf to match the following format:
** [+-]:[+-]n:[+-]yyyy/mm/dd:[+-]yyyy/mm/dd:era_name:era_format
** Store the starting/ending year, month, day, era name, flags, etc.
** into the _era_data structure pointed by sp.
*/

void
store_era(sp, buf)
struct _era_data	*sp;
unsigned char	*buf;
{
	int num, cnt = 0;
	unsigned char *pbuf, *ptmp;

	switch (buf[0]) {
		case '+':
			sp->signflag = 1;
			break;
		case '-':
			sp->signflag = -1;
			break;
		default:
			error(BAD_ERA_FMT);
	}

	if (buf[1] != ':')
		error(BAD_ERA_FMT);

	pbuf = &buf[2];
	num = atoi(pbuf);
	if (num < SHRT_MIN || num > SHRT_MAX)
		error(BAD_ERA_FMT);
	else
		sp->offset = num;
	while (*pbuf && *pbuf++ != ':');

	while (cnt < 2) {
		if (cnt && pbuf[1] == '*') {
			if (pbuf[0] == '+')
				sp->end_year = SHRT_MAX;
			else if (pbuf[0] == '-')
				sp->end_year = SHRT_MIN;
			else
				error(BAD_ERA_FMT);
			pbuf += 2;
		}
		else {
			num = atoi(pbuf);
			if (num < SHRT_MIN+1 || num > SHRT_MAX-1)
				error(BAD_ERA_FMT);
			else
				if (!cnt)
					sp->start_year = num;
				else
					sp->end_year = num;
			while (*pbuf && *pbuf++ != '/');

			num = atoi(pbuf);
			if (num < 0 || num > 12)
				error(BAD_ERA_FMT);
			else
				if (!cnt)
					sp->start_month = num;
				else
					sp->end_month = num;
			while (*pbuf && *pbuf++ != '/');

			num = atoi(pbuf);
			if (num < 0 || num > 31)
				error(BAD_ERA_FMT);
			else
				if (!cnt)
					sp->start_day = num;
				else
					sp->end_day = num;
			while (*pbuf && *pbuf != ':')
				pbuf++;
		}
		if (*pbuf++ != ':')
			error(BAD_ERA_FMT);
		cnt++;
	}


	if ((ptmp = (unsigned char *)malloc((unsigned)20)) == NULL)
		error(NOMEM);
	else {						/* store name string */
		sp->name = ptmp;
		while (*pbuf && *pbuf != ':')
			*ptmp++ = *pbuf++;
		*ptmp = '\0';
	}

	if (!*pbuf)
		sp->format = (unsigned char *)0;
	else {
		if ((ptmp = (unsigned char *)malloc((unsigned)20)) == NULL)
			error(NOMEM);
		else {					/* store format string */
			sp->format = ptmp;
			(void) strcpy(ptmp, ++pbuf);
		}
	}

	sp->origin_year = sp->start_year;

	if (sp->start_year > sp->end_year || (sp->start_year == sp->end_year &&
	(sp->start_month > sp->end_month || (sp->start_month == sp->end_month &&
	sp->start_day > sp->end_day)))) {

		/* start date comes after end date - reverse everything */
		sp->signflag *= -1;
		sp->start_year = sp->end_year;
		sp->end_year = sp->origin_year;
		num = sp->start_month;
		sp->start_month = sp->end_month;
		sp->end_month = num;
		num = sp->start_day;
		sp->start_day = sp->end_day;
		sp->end_day = num;
	}

	if (strlen(sp->name) > 19 || strlen(sp->format) > 19)
		error(ERA_LEN);
}
