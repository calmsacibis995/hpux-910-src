/* @(#) $Revision: 70.5 $ */    
/* LINTLIBRARY */

#include	<stdio.h>
#include	<limits.h>
#include	"global.h"

int	gotstr;

/* mod_init: initialize the process of collation modifier.
** Get here when you see a 'modifier' keyword.
*/
void
mod_init(token)
int token;				/* keyword token */
{
	extern void mod_str();
	extern void mod_finish();

	if (op) {
		/* if we're not the first, finish up last keyword */
		if (finish == NULL) error(STATE);
		(*finish)();
	}

	op = token;			/* save off the token */
	string = mod_str;		/* set sting function for coll_mod */
	finish = mod_finish;		/* set up coll_mod finish */

	gotstr = FALSE;			/* no modifier string yet */
}

/* mod_str: process collation modifier string.
** Check length of the modifier string, and remove quote characters.
** Write header and data out to a tmp file if necessary.
*/
void
mod_str()
{
	extern char *strcpy(),
		    *malloc();
	extern void getstr(),
		    rewind(),
		    col_finish();
	extern void flc_all(),
		    flc_coll(),
		    flc_ctype(),
		    flc_mntry(),
		    flc_nmrc(),
		    flc_time(),
		    plc_all(),
		    plc_coll(),
		    plc_ctype(),
		    plc_mntry(),
		    plc_nmrc(),
		    plc_time(),
		    dlc_all(),
		    dlc_coll(),
		    dlc_ctype(),
		    dlc_mntry(),
		    dlc_nmrc(),
		    dlc_time(),
		    clc_all(),
		    clc_coll(),
		    clc_ctype(),
		    clc_mesg(),
		    clc_mntry(),
		    clc_nmrc(),
		    clc_time();

	unsigned char buf[LEN_INFO_MSGS];
	FILE *fp;

	if (gotstr) error(STATE);		/* only one string allowed */

	(void) getstr(buf);			/* process modifier string */
	if (strlen(buf) > _POSIX_NAME_MAX+2)	/* check length */
		error(MOD_LEN);

	if (!gotinfo) {				/* default modifier with name */
		(void) strcpy(cmlog[cur_cat].cm[0].mod_name, (char *)buf);
	}
	else {					/* additional modifier */
		/* write the previous category/modifier out to a temp file */
		cmlog[cur_cat].n_mod++;
		fp = tmpfile();	
		cmlog[cur_cat].cm[cur_mod].tmp = fp;

		switch (cur_cat) {
			case LC_ALL:
				flc_all();	/* fill in section header */
				plc_all(fp, cur_mod-1);	/* put out header & data */
				dlc_all();	/* write debuging info */
				clc_all();	/* clear data storage space */
				break;
			case LC_COLLATE:
				col_finish();
				flc_coll();
				plc_coll(fp, cur_mod-1);
				dlc_coll();
				clc_coll();
				break;
			case LC_CTYPE:
				flc_ctype();
				plc_ctype(fp, cur_mod-1);
				dlc_ctype();
				clc_ctype();
				break;
			case LC_MONETARY:
				flc_mntry();
				plc_mntry(fp, cur_mod-1);
				dlc_mntry();
				clc_mntry();
				break;
			case LC_NUMERIC:
				flc_nmrc();
				plc_nmrc(fp, cur_mod-1);
				dlc_nmrc();
				clc_nmrc();
				break;
			case LC_TIME:
				flc_time();
				plc_time(fp, cur_mod-1);
				dlc_time();
				clc_time();
				break;
			case LC_MESSAGES:
				flc_mesg();
				plc_messages(fp, cur_mod-1);
				dlc_messages();
				clc_mesg();
				break;
		}
		fflush(fp);
		rewind(fp);			/* rewind for read later */

		cur_mod++;
		(void) strcpy(cmlog[cur_cat].cm[cur_mod].mod_name, (char *)buf);
	}

	gotstr = TRUE;				/* have one string */
}

/* mod_finish: finish up any left-overs from last keyword.
** Called when next keyword or end of file appears.
** Just make sure that there are no meta-chars and one name string. 
*/
void
mod_finish()
{
/*	if (META) error(EXPR); */

	if (!gotstr) {
		lineno--;
		error(STATE);
	}
}

/* clc_all:
** clear/reset the LC_ALL data storage space
*/
void
clc_all()
{
	info_tab[YESSTR] = NULL;
	info_tab[NOSTR] = NULL;
	info_tab[ALT_PUNCT] = NULL;
	info_tab[DIRECTION] = NULL;
}

/* clc_mesg: Posix.2
** clear/reset the LC_MESSAGES data storage space
*/
void
clc_mesg()
{
	info_tab[YESEXPR] = NULL;
	info_tab[NOEXPR] = NULL;

}
/* clc_coll:
** clear/reset the LC_COLLATE data storage space
*/
void
clc_coll()
{
	int i;

	for (i = 0; i < TOT_ELMT; i++) {
		seq_tab[i].seq_no = UNDEF;
		seq_tab[i].type_info = DC;
	}
	two1_head = two1_tail = NULL;
}

/* clc_ctype:
** clear/reset the LC_CTYPE data storage space
*/
void
clc_ctype()
{
	int i;

	for (i = 0; i < CTYPESIZ; i++) {
		ctype1[i] = 0;
		ctype2[i] = 0;
		ctype3[i] = 0;
		ctype4[i] = 0;
		ctype5[i] = 0;
	}

	for (i = 0; i < SHIFTSIZ; i++) {
		upper[i] = i;
		lower[i] = i;
	}

	info_tab[BYTES_CHAR] = NULL;
}

/* clc_mntry:
** clear/reset the LC_MONETARY data storage space
*/
void
clc_mntry()
{
	int i;

	for (i = 1; i <= MNTRY_MSGS; i++)
		mntry_tab[i] = NULL;
}

/* clc_nmrc:
** clear/reset the LC_NUMERIC data storage space
*/
void
clc_nmrc()
{
	int i;

	for (i = 1; i <= NMRC_MSGS; i++)
		nmrc_tab[i] = NULL;

	info_tab[ALT_DIGIT] = NULL;
}

/* clc_time:
** clear/reset the LC_TIME data storage space
*/
void
clc_time()
{
	int i;

	info_tab[D_T_FMT] = NULL;
	info_tab[D_FMT] = NULL;
	info_tab[T_FMT] = NULL;
	for (i = DAY_1; i <= ABMON_12; i++)
		info_tab[i] = NULL;
	info_tab[AM_STR] = NULL;
	info_tab[PM_STR] = NULL;
	for (i = YEAR_UNIT; i <= SEC_UNIT; i++)
		info_tab[i] = NULL;
	info_tab[ERA_FMT] = NULL;

	for (i = 0; i < era_num; i++) {
		era_tab[i] = (struct _era_data *)NULL;
	}
}
