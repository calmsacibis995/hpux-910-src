/*
 * @(#)cdfsdir.h: $Revision: 1.5.83.3 $ $Date: 93/09/17 16:37:18 $
 * $Locker:  $
 */

/* @(#) $Revision: 1.5.83.3 $ */     

#ifndef _SYS_CDFSDIR_INCLUDED /* allows multiple inclusion */
#define _SYS_CDFSDIR_INCLUDED
#define	SECTOR_SIZ	2048	/*smallest addressable part or the recorded
				  area on a CD-ROM that can be addressable
				  independently of other addressable parts of 
				  the recorded area */
#define CDMAXNAMLEN	(30+2+5) /*Max. length of file identifier;
					30 for file name and file extension, 
					2 for seperator, 5 for version number*/
#define CDMINNAMLEN	1	/*Min. length of file identifier;
					1 for file name and file extension */

#define CDROM_IS_DIR    2	/*  mincdd_flag bit #1 set --> this is a dir  */
#define MAX_CDDIR_RECLEN 256	/* Max number of bytes for a directory record*/

struct	min_cddir {
	u_char	mincdd_reclen;	/*length of directory record in bytes*/
	u_char	mincdd_xar_len;	/*length of XAR in logic blocks*/
	/*	The next elements have to be char arrays to 
	**	avoid being padded out of position on the
	**	series 800
	*/
	char	mincdd_loc_lsb[4];	/*logic block number of the extent in LSB*/
	char	mincdd_loc_msb[4];	/*logic block number of the extent in MSB*/
	char	mincdd_size_lsb[4];	/*size (in bytes) of the file section in LSB*/
	char	mincdd_size_msb[4];	/*size (in bytes) of the file section in MSB*/
/*Since date/time info use 7 bytes, I can't put them in a struct (compiler
  round it up to even boundary.  If for any reason this structure is changed,
  all occurance of it should be changed.  Note that this structure is for
  files and directories, the date/time structure in volume descriptors are
  different*/
	u_char	mincdd_year;	/*years since 1900*/
	u_char	mincdd_month;	/*month*/
	u_char	mincdd_day;	/*day*/
	u_char	mincdd_hour;	/*hour*/
	u_char	mincdd_minute;	/*minute*/
	u_char	mincdd_second;	/*second*/
	char	mincdd_timezone;	/*timezone, offset from Greenwich Mean Time 
				  in number of 15 minutes intervals from
				  -48(West) to -52(East)*/
	u_char	mincdd_flag;
	u_char	mincdd_unit_size; /*size (in logic blocks) of file unit*/
	u_char	mincdd_lg_size; /*size (in logic blocks) of interleave gap*/
	u_short mincdd_vol_seq_lsb; /*sequence num. of disc has the extent(LSB)*/
	u_short mincdd_vol_seq_msb; /*sequence num. of disc has the extent(MSB)*/
	u_char	mincdd_idlen;	/*file id length in bytes*/
	char	mincdd_file_id[CDMINNAMLEN];
};
struct cddir {
	struct min_cddir cdd_min;
	char	cdd_rest_id[CDMAXNAMLEN-CDMINNAMLEN];
};
#define	cdd_reclen	cdd_min.mincdd_reclen
#define	cdd_xar_len	cdd_min.mincdd_xar_len
#define	cdd_loc		cdd_min.mincdd_loc_msb
#define	cdd_size	cdd_min.mincdd_size_msb
#define	cdd_year	cdd_min.mincdd_year
#define	cdd_month	cdd_min.mincdd_month
#define	cdd_day		cdd_min.mincdd_day
#define	cdd_hour	cdd_min.mincdd_hour
#define	cdd_minute	cdd_min.mincdd_minute
#define	cdd_second	cdd_min.mincdd_second
#define	cdd_timezone	cdd_min.mincdd_timezone
#define	cdd_flag	cdd_min.mincdd_flag
#define	cdd_unit_size	cdd_min.mincdd_unit_size
#define	cdd_lg_size	cdd_min.mincdd_lg_size
#define	cdd_vol_seq	cdd_min.mincdd_vol_seq_msb
#define	cdd_idlen	cdd_min.mincdd_idlen
#define	cdd_file_id	cdd_min.mincdd_file_id
/*cd_flags*/
#define CD_ASSO		0x4		/*association files*/
#define CD_LE		0x80		/*last extend of a file*/

#define	YEAR_DIGIT	4
#define	MONTH_DIGIT	2
#define	DAY_DIGIT	2
#define	HOUR_DIGIT	2
#define	MINUTE_DIGIT	2
#define	SECOND_DIGIT	2
#define	ZONE_DIGIT	1

struct	cdxar_iso {
	u_short	xar_uid_lsb;
	u_short	xar_uid_msb;
	u_short	xar_gid_lsb;
	u_short	xar_gid_msb;
	u_short	xar_perm;
	char	xar_create_year[YEAR_DIGIT];
	char	xar_create_month[MONTH_DIGIT];
	char	xar_create_day[DAY_DIGIT];
	char	xar_create_hour[HOUR_DIGIT];
	char	xar_create_minute[MINUTE_DIGIT];
	char	xar_create_second[SECOND_DIGIT];
	char	xar_create_centsecond[SECOND_DIGIT];
	char	xar_create_zone[ZONE_DIGIT];
	char	xar_mod_year[YEAR_DIGIT];
	char	xar_mod_month[MONTH_DIGIT];
	char	xar_mod_day[DAY_DIGIT];
	char	xar_mod_hour[HOUR_DIGIT];
	char	xar_mod_minute[MINUTE_DIGIT];
	char	xar_mod_second[SECOND_DIGIT];
	char	xar_mod_centsecond[SECOND_DIGIT];
	char	xar_mod_zone[ZONE_DIGIT];
	char	xar_exp_year[YEAR_DIGIT];
	char	xar_exp_month[MONTH_DIGIT];
	char	xar_exp_day[DAY_DIGIT];
	char	xar_exp_hour[HOUR_DIGIT];
	char	xar_exp_minute[MINUTE_DIGIT];
	char	xar_exp_second[SECOND_DIGIT];
	char	xar_exp_centsecond[SECOND_DIGIT];
	char	xar_exp_zone[ZONE_DIGIT];
	char	xar_eff_year[YEAR_DIGIT];
	char	xar_eff_month[MONTH_DIGIT];
	char	xar_eff_day[DAY_DIGIT];
	char	xar_eff_hour[HOUR_DIGIT];
	char	xar_eff_minute[MINUTE_DIGIT];
	char	xar_eff_second[SECOND_DIGIT];
	char	xar_eff_centsecond[SECOND_DIGIT];
	char	xar_eff_zone[ZONE_DIGIT];
	/*actually longer. */ 
};
struct	cdxar_hsg {
	u_short	xar_uid_lsb;
	u_short	xar_uid_msb;
	u_short	xar_gid_lsb;
	u_short	xar_gid_msb;
	u_short	xar_perm;
	char	xar_create_year[YEAR_DIGIT];
	char	xar_create_month[MONTH_DIGIT];
	char	xar_create_day[DAY_DIGIT];
	char	xar_create_hour[HOUR_DIGIT];
	char	xar_create_minute[MINUTE_DIGIT];
	char	xar_create_second[SECOND_DIGIT];
	char	xar_create_centsecond[SECOND_DIGIT];
	char	xar_mod_year[YEAR_DIGIT];
	char	xar_mod_month[MONTH_DIGIT];
	char	xar_mod_day[DAY_DIGIT];
	char	xar_mod_hour[HOUR_DIGIT];
	char	xar_mod_minute[MINUTE_DIGIT];
	char	xar_mod_second[SECOND_DIGIT];
	char	xar_mod_centsecond[SECOND_DIGIT];
	char	xar_exp_year[YEAR_DIGIT];
	char	xar_exp_month[MONTH_DIGIT];
	char	xar_exp_day[DAY_DIGIT];
	char	xar_exp_hour[HOUR_DIGIT];
	char	xar_exp_minute[MINUTE_DIGIT];
	char	xar_exp_second[SECOND_DIGIT];
	char	xar_exp_centsecond[SECOND_DIGIT];
	char	xar_eff_year[YEAR_DIGIT];
	char	xar_eff_month[MONTH_DIGIT];
	char	xar_eff_day[DAY_DIGIT];
	char	xar_eff_hour[HOUR_DIGIT];
	char	xar_eff_minute[MINUTE_DIGIT];
	char	xar_eff_second[SECOND_DIGIT];
	char	xar_eff_centsecond[SECOND_DIGIT];
	/*actually longer. */ 
};
#define	xar_uid	xar_uid_msb;
#define	xar_gid	xar_gid_msb;
#endif /* _SYS_CDFSDIR_INCLUDED */
