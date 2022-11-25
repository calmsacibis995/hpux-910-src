/* @(#) $Revision: 66.1 $ */      
/* LINTLIBRARY */

#include "global.h"

/* debug_locale:
** write 'locale.def' file information into the 'debug' file.
** for debugging purpose only.
*/
void
debug_locale()
{
	extern void dlc_all(),
		    dlc_coll(),
		    dlc_ctype(),
		    dlc_mntry(),
		    dlc_nmrc(),
		    dlc_time();

#ifdef LCDBG
	register int i, j;

	/*
	** write information in CMLOG
	*/
	(void) fprintf(dfp, "\nCMLOG");
	for (i = 0; i < N_CATEGORY; i++) {
		(void) fprintf(dfp, "\ncategory: %d\t", i);
		(void) fprintf(dfp, "n_mod: %d\n", cmlog[i].n_mod);
		for (j = 0; j < cmlog[i].n_mod; j++) {
		    (void) fprintf(dfp, "mod_name: %s\n",
					 cmlog[i].cm[j].mod_name);
		    (void) fprintf(dfp, "tmp: %x\n", cmlog[i].cm[j].tmp);
		    (void) fprintf(dfp, "size: %d\n", cmlog[i].cm[j].size);
		    (void) fprintf(dfp, "pad: %d\n", cmlog[i].cm[j].pad);
		}
	}

	/*
	** write information in LOCALE TABLE HEADER
	*/
	(void) fprintf(dfp, "\nLOCALE TABLE HEADER\n");
	(void) fprintf(dfp, "size: %d\n", lctable_head.size);
	(void) fprintf(dfp, "id: %d,\tname: %s\n",
		      lctable_head.nl_langid, lctable_head.lang);
	(void) fprintf(dfp, "cat_no: %d,\tmod_no: %d\n",
		      lctable_head.cat_no, lctable_head.mod_no);
	(void) fprintf(dfp, "rev_flag: %d,\trev_str: %s\n",
		      lctable_head.rev_flag, lctable_head.rev_str);
	(void) fprintf(dfp, "codeset: %d\n", lctable_head.codeset);

	/*
	** write information in CATEGORY STRUCTURE
	*/
	(void) fprintf(dfp, "\nCATEGORY STRUCTURE\n");
	for (i = 0; i < N_CATEGORY; i++) {
		(void) fprintf(dfp, "size: %d\n", catinfo[i].size);
		(void) fprintf(dfp, "addr: %d\n", catinfo[i].addr);
		(void) fprintf(dfp, "mod_name: %s\n", catinfo[i].mod_name);
		(void) fprintf(dfp, "catid: %d\n", catinfo[i].catid);
		(void) fprintf(dfp, "mod_addr: %d\n\n", catinfo[i].mod_addr);
	}

	/*
	** write information in MODIFIER STRUCTURE
	*/
	(void) fprintf(dfp, "\nMODIFIER STRUCTURE\n");
	for (i = (N_CATEGORY); i < (N_CATEGORY+lctable_head.mod_no); i++) {
		(void) fprintf(dfp, "size: %d\n", catinfo[i].size);
		(void) fprintf(dfp, "addr: %d\n", catinfo[i].addr);
		(void) fprintf(dfp, "mod_name: %s\n", catinfo[i].mod_name);
		(void) fprintf(dfp, "catid: %d\n", catinfo[i].catid);
		(void) fprintf(dfp, "mod_addr: %d\n\n", catinfo[i].mod_addr);
	}
#endif

	/*
	** write information in each categories
	*/
	dlc_all();
	dlc_coll();
	dlc_ctype();
	dlc_mntry();
	dlc_nmrc();
	dlc_time();
}

/* dlc_all:
** write LC_ALL category information into the 'debug' file.
*/
void
dlc_all()
{
#ifdef ALLDBG
	(void) fprintf(dfp, "\nLC_ALL HEADER\n");
	(void) fprintf(dfp, "yes_addr: %d\n", lcall_head.yes_addr);
	(void) fprintf(dfp, "no_addr: %d\n", lcall_head.no_addr);
	(void) fprintf(dfp, "direct_addr: %d\n", lcall_head.direct_addr);
	(void) fprintf(dfp, "context_addr: %d\n", lcall_head.context_addr);

	(void) fprintf(dfp, "\nLC_ALL DATA\n");
	(void) fprintf(dfp, "yesstr: %s\n", info_tab[YESSTR]);
	(void) fprintf(dfp, "nostr: %s\n", info_tab[NOSTR]);
	(void) fprintf(dfp, "direction: %s\n", info_tab[DIRECTION]);
	(void) fprintf(dfp, "context: %s\n", info_tab[CONTEXT]);
#endif
}

/* dlc_coll:
** write LC_COLLATE category information into the 'debug' file.
*/
void
dlc_coll()
{
#ifdef	COLLDBG
	register int i, j;

	(void) fprintf(dfp, "\nCOLLATE HEADER\n");
	(void) fprintf(dfp, "seqtab_addr: %d\n", lccol_head.seqtab_addr);
	(void) fprintf(dfp, "pritab_addr: %d\n", lccol_head.pritab_addr);
	(void) fprintf(dfp, "two1tab_addr: %d\n", lccol_head.two1tab_addr);
	(void) fprintf(dfp, "one2tab_addr: %d\n", lccol_head.one2tab_addr);
	(void) fprintf(dfp, "nl_map21: %d\n", lccol_head.nl_map21);
	(void) fprintf(dfp, "nl_onlyseq: %d\n", lccol_head.nl_onlyseq);

	(void) fprintf(dfp, "\nCOLLATE DATA\n");
	(void) fprintf(dfp, "sequence\n");
	for (i = 0; i < TOT_ELMT; ) {
		for (j = 0; j < 16; j++)
			(void) fprintf(dfp, "%d ",
				seq_tab[i++].seq_no);
		(void) fprintf(dfp, "\n");
	}
	(void) fprintf(dfp, "\nflag+priority\n");
	for (i = 0; i < TOT_ELMT; ) {
		for (j = 0; j < 16; j++)
			(void) fprintf(dfp, "%d ",
				seq_tab[i++].type_info);
		(void) fprintf(dfp, "\n");
	}
	(void) fprintf(dfp, "\n2-1\n");
	for (i = 0; i < 16; i++) {
		(void) fprintf(dfp, " (%d %c %d %d)\n",
				two1_tab[i].reserved,
				two1_tab[i].legal,
				two1_tab[i].seq2.seq_no,
				two1_tab[i].seq2.type_info);
	}
	(void) fprintf(dfp, "\n1-2\n");
	for (i = 0; i < 16; i++) {
		(void) fprintf(dfp, " (%d %d)\n",
				one2_tab[i].seq_no,
				one2_tab[i].type_info);
	}
#endif
}

/* dlc_ctype:
** write LC_CTYPE category information into the 'debug' file.
*/
void
dlc_ctype()
{
#ifdef CTYPEDBG
	register int i, j;

	(void) fprintf(dfp, "\nCTYPE HEADER\n");
	(void) fprintf(dfp, "sh_high: %d,\tsh_low: %d\n",
		      lcctype_head.sh_high, lcctype_head.sh_low);
	(void) fprintf(dfp, "ctype_addr: %d\n", lcctype_head.ctype_addr);
	(void) fprintf(dfp, "kanji1_addr: %d\nkanji2_addr: %d\n",
		      lcctype_head.kanji1_addr, lcctype_head.kanji2_addr);
	(void) fprintf(dfp, "upshift_addr: %d\ndownshift_addr: %d\n",
		      lcctype_head.upshift_addr, lcctype_head.downshift_addr);
	(void) fprintf(dfp, "byte_char_addr: %d\n",
		      lcctype_head.byte_char_addr);
	(void) fprintf(dfp, "alt_punct_addr: %d\n",
		      lcctype_head.alt_punct_addr);
#ifdef EUC
	(void) fprintf(dfp, "io_csize_addr: %d\n", lcctype_head.io_csize_addr);
	(void) fprintf(dfp, "e_cset_addr: %d\n", lcctype_head.e_cset_addr);
	(void) fprintf(dfp, "ein_csize_addr: %d\n",
		      lcctype_head.ein_csize_addr);
	(void) fprintf(dfp, "eout_csize_addr: %d\n",
		      lcctype_head.eout_csize_addr);
#endif /* EUC */

	(void) fprintf(dfp, "\nCTYPE DATA\n");
	(void) fprintf(dfp, "ctype\n");
	(void) fprintf(dfp, "%x\n",ctype1[0]);
	for (i = 1; i < CTYPESIZ; ) {
		for (j = 0; j < 16; j++)
			(void) fprintf(dfp, "%x ",ctype1[i++]);
		(void) fprintf(dfp, "\n");
	}
	(void) fprintf(dfp, "\nKANJI1\n");
	(void) fprintf(dfp, "%x\n",ctype3[0]);
	for (i = 1; i < CTYPESIZ; ) {
		for (j = 0; j < 16; j++)
			(void) fprintf(dfp, "%x ",ctype3[i++]);
		(void) fprintf(dfp, "\n");
	}
	(void) fprintf(dfp, "\nKANJI2\n");
	(void) fprintf(dfp, "%x\n",ctype4[0]);
	for (i = 1; i < CTYPESIZ; ) {
		for (j = 0; j < 16; j++)
			(void) fprintf(dfp, "%x ",ctype4[i++]);
		(void) fprintf(dfp, "\n");
	}

	(void) fprintf(dfp, "\nUPPER\n");
	for (i = 0; i < SHIFTSIZ; ) {
		for (j = 0; j < 16; j++)
			(void) fprintf(dfp, "%x ",upper[i++]);
		(void) fprintf(dfp, "\n");
	}
	(void) fprintf(dfp, "\nLOWER\n");
	for (i = 0; i < SHIFTSIZ; ) {
		for (j = 0; j < 16; j++)
			(void) fprintf(dfp, "%x ",lower[i++]);
		(void) fprintf(dfp, "\n");
	}

	(void) fprintf(dfp, "bytes_char: %s\n", info_tab[BYTES_CHAR]);
	(void) fprintf(dfp, "alt_punct: %s\n", info_tab[ALT_PUNCT]);

#ifdef EUC
	(void) fprintf(dfp, "\nIO_CHARSIZE\n");
	for (i = 0; i < MAX_CODESET*2; )
		(void) fprintf(dfp, "%d ", io_charsize[i++]);
	(void) fprintf(dfp, "\n");

	(void) fprintf(dfp, "\nE_CSET\n");
	for (i = 0; i < SHIFTSIZ; ) {
		for (j = 0; j < 16; j++)
			(void) fprintf(dfp, "%x ", e_cset[i++]);
		(void) fprintf(dfp, "\n");
	}

	(void) fprintf(dfp, "\nEIN_CSIZE\n");
	for (i = 0; i < SHIFTSIZ; ) {
		for (j = 0; j < 16; j++)
			(void) fprintf(dfp, "%x ", ein_csize[i++]);
		(void) fprintf(dfp, "\n");
	}

	(void) fprintf(dfp, "\nEOUT_CSIZE\n");
	for (i = 0; i < SHIFTSIZ; ) {
		for (j = 0; j < 16; j++)
			(void) fprintf(dfp, "%x ", eout_csize[i++]);
		(void) fprintf(dfp, "\n");
	}
#endif /* EUC */
#endif
}

/* dlc_mntry:
** write LC_MONETARY category information into the 'debug' file.
*/
void
dlc_mntry()
{
#ifdef MONDBG
	(void) fprintf(dfp, "\nMONETARY HEADER (addresses)\n");
	(void) fprintf(dfp, "int_frac_digits: %d\n",
				lcmonetary_head.int_frac_digits);
	(void) fprintf(dfp, "frac_digits: %d\n",
				lcmonetary_head.frac_digits);
	(void) fprintf(dfp, "p_cs_precedes: %d\n",
				lcmonetary_head.p_cs_precedes);
	(void) fprintf(dfp, "p_sep_by_space: %d\n",
				lcmonetary_head.p_sep_by_space);
	(void) fprintf(dfp, "n_cs_precedes: %d\n",
				lcmonetary_head.n_cs_precedes);
	(void) fprintf(dfp, "n_sep_by_space: %d\n",
				lcmonetary_head.n_sep_by_space);
	(void) fprintf(dfp, "p_sign_posn: %d\n",
				lcmonetary_head.p_sign_posn);
	(void) fprintf(dfp, "n_sign_posn: %d\n",
				lcmonetary_head.n_sign_posn);
	(void) fprintf(dfp, "currency_symbol_lc: %d\n",
				lcmonetary_head.currency_symbol_lc);
	(void) fprintf(dfp, "currency_symbol_li: %d\n",
				lcmonetary_head.currency_symbol_li);
	(void) fprintf(dfp, "mon_decimal_point: %d\n",
				lcmonetary_head.mon_decimal_point);
	(void) fprintf(dfp, "int_curr_symbol: %d\n",
				lcmonetary_head.int_curr_symbol);
	(void) fprintf(dfp, "mon_thousands_sep: %d\n",
				lcmonetary_head.mon_thousands_sep);
	(void) fprintf(dfp, "mon_grouping: %d\n",
				lcmonetary_head.mon_grouping);
	(void) fprintf(dfp, "positive_sign: %d\n",
				lcmonetary_head.positive_sign);
	(void) fprintf(dfp, "negative_sign: %d\n",
				lcmonetary_head.negative_sign);

	(void) fprintf(dfp, "\nMONETARY DATA\n");
	(void) fprintf(dfp, "int_frac_digits: %s\n", mntry_tab[INT_FRAC]);
	(void) fprintf(dfp, "frac_digits: %s\n", mntry_tab[FRAC_DIGITS]);
	(void) fprintf(dfp, "p_cs_precedes: %s\n", mntry_tab[P_CS]);
	(void) fprintf(dfp, "p_sep_by_space: %s\n", mntry_tab[P_SEP]);
	(void) fprintf(dfp, "n_cs_precedes: %s\n", mntry_tab[N_CS]);
	(void) fprintf(dfp, "n_sep_by_space: %s\n", mntry_tab[N_SEP]);
	(void) fprintf(dfp, "p_sign_posn: %s\n", mntry_tab[P_SIGN]);
	(void) fprintf(dfp, "n_sign_posn: %s\n", mntry_tab[N_SIGN]);
	(void) fprintf(dfp, "currency_symbol_lc: %s\n", mntry_tab[CURRENCY_LC]);
	(void) fprintf(dfp, "currency_symbol_li: %s\n", mntry_tab[CURRENCY_LI]);
	(void) fprintf(dfp, "mon_decimal_point: %s\n", mntry_tab[MON_DECIMAL]);
	(void) fprintf(dfp, "int_curr_symbol: %s\n", mntry_tab[INT_CURR]);
	(void) fprintf(dfp, "mon_thousands_sep: %s\n",mntry_tab[MON_THOUSANDS]);
	(void) fprintf(dfp, "mon_grouping: %s\n", mntry_tab[MON_GROUPING]);
	(void) fprintf(dfp, "positive_sign: %s\n", mntry_tab[POSITIVE_SIGN]);
	(void) fprintf(dfp, "negative_sign: %s\n", mntry_tab[NEGATIVE_SIGN]);
#endif
}

/* dlc_nmrc:
** write LC_NUMERIC category information into the 'debug' file.
*/
void
dlc_nmrc()
{
#ifdef NUMDBG
	(void) fprintf(dfp, "\nNUMERIC HEADER (addresses)\n");
	(void) fprintf(dfp, "grouping: %d\n",
					lcnumeric_head.grouping);
	(void) fprintf(dfp, "decimal_point: %d\n",
					lcnumeric_head.decimal_point);
	(void) fprintf(dfp, "thousands_sep: %d\n",
					lcnumeric_head.thousands_sep);
	(void) fprintf(dfp, "alt_digit_addr: %d\n",
					lcnumeric_head.alt_digit_addr);

	(void) fprintf(dfp, "\nNUMERIC DATA\n");
	(void) fprintf(dfp, "grouping: %s\n", nmrc_tab[GROUPING]);
	(void) fprintf(dfp, "decimal_point: %s\n", nmrc_tab[DECIMAL_P]);
	(void) fprintf(dfp, "thousands_sep: %s\n", nmrc_tab[THOUSANDS_S]);
	(void) fprintf(dfp, "alt_digit_addr: %s\n", info_tab[ALT_DIGIT]);
#endif
}

/* dlc_time:
** write LC_TIME category information into the 'debug' file.
*/
void
dlc_time()
{
#ifdef TIMEDBG
	register int i;

	(void) fprintf(dfp,"\nTIME HEADER (addresses)\n");
	(void) fprintf(dfp,"d_t_fmt: %d\n", lctime_head.d_t_fmt);
	(void) fprintf(dfp,"d_fmt: %d\n", lctime_head.d_fmt);
	(void) fprintf(dfp,"t_fmt: %d\n", lctime_head.t_fmt);
	(void) fprintf(dfp,"day 1-7: %d %d %d %d %d %d %d\n",
			lctime_head.day_1, lctime_head.day_2, lctime_head.day_3,
			lctime_head.day_4, lctime_head.day_5, lctime_head.day_6,
			lctime_head.day_7);
	(void) fprintf(dfp,"abday 1-7: %d %d %d %d %d %d %d\n",
			lctime_head.abday_1, lctime_head.abday_2,
			lctime_head.abday_3, lctime_head.abday_4,
			lctime_head.abday_5, lctime_head.abday_6,
			lctime_head.abday_7);
	(void) fprintf(dfp,"mon 1-12: %d %d %d %d %d %d\n\t%d %d %d %d %d %d\n",
			lctime_head.mon_1, lctime_head.mon_2, lctime_head.mon_3,
			lctime_head.mon_4, lctime_head.mon_5, lctime_head.mon_6,
			lctime_head.mon_7, lctime_head.mon_8, lctime_head.mon_9,
			lctime_head.mon_10, lctime_head.mon_11,
			lctime_head.mon_12);
	(void) fprintf(dfp,
		       "abmon 1-12: %d %d %d %d %d %d\n\t%d %d %d %d %d %d\n",
			lctime_head.abmon_1, lctime_head.abmon_2,
			lctime_head.abmon_3, lctime_head.abmon_4,
			lctime_head.abmon_5, lctime_head.abmon_6,
			lctime_head.abmon_7, lctime_head.abmon_8,
			lctime_head.abmon_9, lctime_head.abmon_10,
			lctime_head.abmon_11, lctime_head.abmon_12);
	(void) fprintf(dfp, "am_str: %d\n", lctime_head.am_str);
	(void) fprintf(dfp, "pm_str: %d\n", lctime_head.pm_str);

	(void) fprintf(dfp, "year_unit: %d\n", lctime_head.year_unit);
	(void) fprintf(dfp, "mon_unit: %d\n", lctime_head.mon_unit);
	(void) fprintf(dfp, "day_unit: %d\n", lctime_head.day_unit);
	(void) fprintf(dfp, "hour_unit: %d\n", lctime_head.hour_unit);
	(void) fprintf(dfp, "min_unit: %d\n", lctime_head.min_unit);
	(void) fprintf(dfp, "sec_unit: %d\n", lctime_head.sec_unit);

	(void) fprintf(dfp, "era_fmt: %d\n", lctime_head.era_fmt);
	(void) fprintf(dfp, "era_count: %d\n", lctime_head.era_count);
	(void) fprintf(dfp, "era_names: %d\n", lctime_head.era_names);
	(void) fprintf(dfp, "era_addr: %d\n", lctime_head.era_addr);

	(void) fprintf(dfp, "\nTIME DATA\n");
	(void) fprintf(dfp, "d_t_fmt: %s\n", info_tab[D_T_FMT]);
	(void) fprintf(dfp, "d_fmt: %s\n", info_tab[D_FMT]);
	(void) fprintf(dfp, "t_fmt: %s\n", info_tab[T_FMT]);
	for (i = DAY_1; i <= ABMON_12; i++)
		(void) fprintf(dfp, "%d %s\n", i, info_tab[i]);
	(void) fprintf(dfp, "am_str: %s\n", info_tab[AM_STR]);
	(void) fprintf(dfp, "pm_str: %s\n", info_tab[PM_STR]);
	for (i = YEAR_UNIT; i <= SEC_UNIT; i++)
		(void) fprintf(dfp, "%d %s\n", i, info_tab[i]);

	(void) fprintf(dfp, "era_fmt: %s\n", info_tab[ERA_FMT]);
	(void) fprintf(dfp, "\nera_count: %d\n", lctime_head.era_count);
	(void) fprintf(dfp, "era table\n");
	for (i = 0; i < lctime_head.era_count; i++) {
		(void) fprintf(dfp, "i: %d\n", i);
		(void) fprintf(dfp, "s_year: %d\ts_month: %d\ts_day: %d\n",
				era_tab[i]->start_year,
				era_tab[i]->start_month,
				era_tab[i]->start_day);
		(void) fprintf(dfp, "e_year: %d\te_month: %d\te_day: %d\n",
				era_tab[i]->end_year,
				era_tab[i]->end_month,
				era_tab[i]->end_day);
		(void) fprintf(dfp, "origin_year: %d\n", era_tab[i]->origin_year);
		(void) fprintf(dfp, "offset: %d\n", era_tab[i]->offset);
		(void) fprintf(dfp, "signflag: %d\n", era_tab[i]->signflag);
		(void) fprintf(dfp, "name: %s\n", era_tab2[2*i]);
		(void) fprintf(dfp, "format: %s\n", era_tab2[2*i+1]);
	}
#endif
}
