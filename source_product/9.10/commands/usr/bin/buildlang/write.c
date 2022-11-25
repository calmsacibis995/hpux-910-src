/* @(#) $Revision: 66.1 $ */      
/* LINTLIBRARY */

#include <stdio.h>
#include <nl_ctype.h>
#include "global.h"

unsigned char null = '\000';

/* plc_all:
** write out the LC_ALL section (header & data)
*/
void
plc_all(fp, mod)
FILE *fp;
int mod;
{
	int i;

	for (i = 0; i < sizeof(lcall_head); i++)
		(void) fprintf(fp, "%c", ((char *)&lcall_head)[i]);

	(void) fprintf(fp, "%s", info_tab[YESSTR]);
	(void) fprintf(fp, "%c", (char)null);
	(void) fprintf(fp, "%s", info_tab[NOSTR]);
	(void) fprintf(fp, "%c", (char)null);
	(void) fprintf(fp, "%s", info_tab[DIRECTION]);
	(void) fprintf(fp, "%c", (char)null);
	(void) fprintf(fp, "%s", info_tab[CONTEXT]);
	(void) fprintf(fp, "%c", (char)null);

	for (i = cmlog[LC_ALL].cm[mod].pad; i; i--)
		(void) fprintf(fp, "%c", (char)null);
}

/* plc_coll:
** write out the LC_COLLATE section (header & data)
*/
void
plc_coll(fp, mod)
FILE *fp;
int mod;
{
	int i;

	if (!LANG2BYTE && collate) {
		for (i = 0; i < sizeof(lccol_head); i++)
			(void) fprintf(fp, "%c", ((char *)&lccol_head)[i]);

		for (i = 0; i < TOT_ELMT; i++)
			(void) fprintf(fp, "%c", (char)seq_tab[i].seq_no);
		for (i = 0; i < TOT_ELMT; i++)
			(void) fprintf(fp, "%c", (char)seq_tab[i].type_info);
		for (i = 0; i < two1_len; i++)
			(void) fprintf(fp, "%c", ((char *)two1_tab)[i]);
		for (i = 0; i < one2_len; i++)
			(void) fprintf(fp, "%c", ((char *)one2_tab)[i]);
	}

	for (i = cmlog[LC_COLLATE].cm[mod].pad; i; i--)
		(void) fprintf(fp, "%c", (char)null);
}

/* plc_ctype:
** write out the LC_CTYPE section (header & data)
*/
void
plc_ctype(fp, mod)
FILE *fp;
int mod;
{
	int i;

	for (i = 0; i < sizeof(lcctype_head); i++)
		(void) fprintf(fp, "%c", ((char *)&lcctype_head)[i]);

	for (i = 0; i < CTYPESIZ; i++) {
		ctype3[i] = ctype2[i] & _K1;
		ctype4[i] = ctype2[i] & (_K1 | _K2);
	}

#ifdef EUC
	if (lctable_head.codeset == CODE_HP15) {
		io_charsize[1] = 2;
		io_charsize[5] = 2;
		if (ctype3[SS2]) {
			io_charsize[2] = 2;
			io_charsize[6] = 2;
		} else {
			io_charsize[2] = 1;
			io_charsize[6] = 1;
		}
		if (ctype3[SS3]) {
			io_charsize[3] = 2;
			io_charsize[7] = 2;
		} else {
			io_charsize[3] = 1;
			io_charsize[7] = 1;
		}
	}

	if (lctable_head.codeset == CODE_EUC) {	    /* for _ECSET macro */
		e_cset[SS2] = 2;
		e_cset[SS3] = 3;
	}

	if (lctable_head.codeset == CODE_EUC)
		for (i = 0; i < SHIFTSIZ; i++) {
			ein_csize[i] = io_charsize[e_cset[i]];
			eout_csize[i] = io_charsize[e_cset[i]+4];
		}
	if (lctable_head.codeset == CODE_HP15)
		for (i = 0; i < SHIFTSIZ; i++)
			ein_csize[i] = eout_csize[i] = ctype3[i+1]+1;

#endif /* EUC */
	for (i = 0; i < CTYPESIZ; i++)
		(void) fprintf(fp, "%c", ctype1[i]);
	for (i = 0; i < CTYPESIZ; i++)
		(void) fprintf(fp, "%c", ctype3[i]);
	for (i = 0; i < CTYPESIZ; i++)
		(void) fprintf(fp, "%c", ctype4[i]);

	for (i = 0; i < SHIFTSIZ; i++)
		(void) fprintf(fp, "%c", upper[i]);
	for (i = 0; i < SHIFTSIZ; i++)
		(void) fprintf(fp, "%c", lower[i]);

	(void) fprintf(fp, "%s", info_tab[BYTES_CHAR]);
	(void) fprintf(fp, "%c", (char)null);
	(void) fprintf(fp, "%s", info_tab[ALT_PUNCT]);
	(void) fprintf(fp, "%c", (char)null);

#ifdef EUC
	for (i = 0; i < MAX_CODESET*2; i++)
		(void) fprintf(fp, "%c", io_charsize[i]);

	for (i = 0; i < SHIFTSIZ; i++)
		(void) fprintf(fp, "%c", e_cset[i]);

	for (i = 0; i < SHIFTSIZ; i++)
		(void) fprintf(fp, "%c", ein_csize[i]);

	for (i = 0; i < SHIFTSIZ; i++)
		(void) fprintf(fp, "%c", eout_csize[i]);

#endif /* EUC */
	for (i = cmlog[LC_CTYPE].cm[mod].pad; i; i--)
		(void) fprintf(fp, "%c", (char)null);
}

/* plc_mntry:
** write out the LC_MONETARY section (header & data)
*/
void
plc_mntry(fp, mod)
FILE *fp;
int mod;
{
	int i;

	for (i = 0; i < sizeof(lcmonetary_head); i++)
		(void) fprintf(fp, "%c", ((char *)&lcmonetary_head)[i]);

	for (i = 1; i <= MNTRY_MSGS; i++) {
		(void) fprintf(fp, "%s", mntry_tab[i]);
		(void) fprintf(fp, "%c", (char)null);
	}

	for (i = cmlog[LC_MONETARY].cm[mod].pad; i; i--)
		(void) fprintf(fp, "%c", (char)null);
}

/* plc_nmrc:
** write out the LC_NUMERIC section (header & data)
*/
void
plc_nmrc(fp, mod)
FILE *fp;
int mod;
{
	int i;

	for (i = 0; i < sizeof(lcnumeric_head); i++)
		(void) fprintf(fp, "%c", ((char *)&lcnumeric_head)[i]);

	(void) fprintf(fp, "%s", nmrc_tab[GROUPING]);
	(void) fprintf(fp, "%c", (char)null);
	(void) fprintf(fp, "%s", nmrc_tab[DECIMAL_P]);
	(void) fprintf(fp, "%c", (char)null);
	(void) fprintf(fp, "%s", nmrc_tab[THOUSANDS_S]);
	(void) fprintf(fp, "%c", (char)null);
	(void) fprintf(fp, "%s", info_tab[ALT_DIGIT]);
	(void) fprintf(fp, "%c", (char)null);

	for (i = cmlog[LC_NUMERIC].cm[mod].pad; i; i--)
		(void) fprintf(fp, "%c", (char)null);
}

/* plc_time:
** write out the LC_TIME section (header & data)
*/
void
plc_time(fp, mod)
FILE *fp;
int mod;
{
	int i, j;

	for (i = 0; i < sizeof(lctime_head); i++)
		(void) fprintf(fp, "%c", ((char *)&lctime_head)[i]);

	(void) fprintf(fp, "%s", info_tab[D_T_FMT]);
	(void) fprintf(fp, "%c", (char)null);
	(void) fprintf(fp, "%s", info_tab[D_FMT]);
	(void) fprintf(fp, "%c", (char)null);
	(void) fprintf(fp, "%s", info_tab[T_FMT]);
	(void) fprintf(fp, "%c", (char)null);

	for (i = DAY_1; i <= ABMON_12; i++) {
		(void) fprintf(fp, "%s", info_tab[i]);
		(void) fprintf(fp, "%c", null);
	}

	(void) fprintf(fp, "%s", info_tab[AM_STR]);
	(void) fprintf(fp, "%c", (char)null);
	(void) fprintf(fp, "%s", info_tab[PM_STR]);
	(void) fprintf(fp, "%c", (char)null);

	for (i = YEAR_UNIT; i <= SEC_UNIT; i++) {
		(void) fprintf(fp, "%s", info_tab[i]);
		(void) fprintf(fp, "%c", null);
	}

	(void) fprintf(fp, "%s", info_tab[ERA_FMT]);
	(void) fprintf(fp, "%c", (char)null);

	for (i = 0; i < 2*era_num; i++) {
		(void) fprintf(fp, "%s", (char *)era_tab2[i]);
		(void) fprintf(fp, "%c", (char)null);
	}

	for (i = pad_byte; i; i--)
		(void) fprintf(fp, "%c", (char)null);
	for (i = 0; i < era_num; i++) {
		for (j = 0; j < sizeof(struct _era_data); j++)
			(void) fprintf(fp, "%c", ((char *)era_tab[i])[j]);
	}

	for (i = cmlog[LC_TIME].cm[mod].pad; i; i--)
		(void) fprintf(fp, "%c", (char)null);
}
