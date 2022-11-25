/*
 * @(#)cdfs.h: $Revision: 1.6.83.4 $ $Date: 94/10/14 12:21:03 $
 * $Locker:  $
 */

/* @(#) $Revision: 1.6.83.4 $ */     
#ifndef _SYS_CDFS_INCLUDED /* allows multiple inclusion */
#define _SYS_CDFS_INCLUDED

#define CDSBSIZE		2048	/*sector size of CDROM*/
#define	CDSBLOCK		16

#define	VOL_SET_ID_SIZ	128
#define	VOL_ID_SIZ	32
#define	STD_ID_SIZ	5
#define	SYS_ID_SIZ	32
#define	PUBLISHER_ID_SIZ	128
#define	PREPARER_ID_SIZ	128
#define	APPLICATION_ID_SIZ	128
#define	APPL_USE_SIZ	512

#ifndef MAXMNTLEN
#define MAXMNTLEN	512	/* keep in sync with fs.h & filsys.h */
#endif

/*volume descriptor type*/
#define	VOL_BOOT		0
#define	VOL_PRIMARY		1
#define	VOL_SUPPLEMENTRAY	2
#define	VOL_PARTITION		3
#define	VOL_TERMINATE		255

#define CDFS_MAGIC_HSG	0x95411
#define CDFS_MAGIC_ISO	0x19450

/*size of structures for  all kinds of vol descriptor should be KSBIZE*/
struct	icddfs			/*primary volume descriptor-ISO-9660*/
{
	char cdrom_vold_type;	/*volume descriptor type*/
	char cdrom_std_id[STD_ID_SIZ];	/*id "CD001" for ISO-9660 */
	char cdrom_vold_version;	/*should be 1 for ISO-9660 */
	char cdrom_unused1;	/*spare*/
	char cdrom_sys_id[SYS_ID_SIZ];	/*id of a system that knows contents of 
				  system area, logic sector 0-15*/
	char cdrom_vol_id[VOL_ID_SIZ];	/*id of this volume*/
	char cdrom_unused2[8];	/*spare*/
	int  cdrom_vol_size_lsb;	/*size (LSB) of volume in logic block*/
	int  cdrom_vol_size_msb;	/*size (MSB) of volume in logic block*/
	char cdrom_unused3[32];	/*spare*/
	ushort cdrom_volset_siz_lsb; /*size (LSB) of volume set*/
	ushort cdrom_volset_siz_msb; /*size (MSB) of volume set*/
	ushort cdrom_volset_seq_lsb; /*sequence number (LSB) of volume in the set*/
	ushort cdrom_volset_seq_msb; /*sequence number (MSB) of volume in the set*/
	ushort cdrom_logblk_siz_lsb; /*size of (LSB) of logic block in bytes*/
	ushort cdrom_logblk_siz_msb; /*size of (MSB) of logic block in bytes*/
	u_int  cdrom_pathtbl_siz_lsb; /*size (LSB) of path table in bytes */
	u_int  cdrom_pathtbl_siz_msb; /*size (MSB) of path table in bytes */
	u_int  cdrom_pathtbl_loc_lsb; /*logical block number(LSB) of path table*/
	u_int  cdrom_pathtblo_loc_lsb; /*logical block number(LSB) of 
					optional path table*/
	u_int  cdrom_pathtbl_loc_msb; /*logic block num.(MSB) of path table*/
	u_int  cdrom_pathtblo_loc_msb; /*logic block num(MSB) of optional path table*/
	struct min_cddir	cdrom_rootdp; /*directory record of root*/
	char	cdrom_vol_set_id[VOL_SET_ID_SIZ];	/*id of the volume set*/
	char	cdrom_pb_id[PUBLISHER_ID_SIZ];	/*publisher's id*/
	char	cdrom_pp_id[PREPARER_ID_SIZ];	/*preparer's id*/
	char	cdrom_ap_id[APPLICATION_ID_SIZ];	/*application id*/
	char	cdrom_copyright[CDMAXNAMLEN];	/*copyright in this file under
						  root directory, the max. 
						  len. is 18, the rest unused*/
	char	cdrom_abstract[CDMAXNAMLEN];	/*abstract in this file under
						  root directory, the max. 
						  len. is 18, the rest unused*/
	char	cdrom_bibliographic[CDMAXNAMLEN];	/*bibliographic in this file 
						  under root directory, the max.
						  len. is 18, the rest unused*/
/*the nest four chunks are creation time, modificaton time, expiration time 
  and effective time.  Since the date/time info uses 17 bytes(odd), I can't
  use a structure (compiler round up to even bytes).  If for any reason this
  info is changed, make sure to fix all four of them.*/
/*creation time*/
	char	cdrom_c_year[4];	/*years since year 0000*/
	char	cdrom_c_month[2];	/*month*/
	char	cdrom_c_day[2];	/*day*/
	char	cdrom_c_hour[2];	/*hour*/
	char	cdrom_c_minute[2];	/*minute*/
	char	cdrom_c_second[2];	/*second*/
	char	cdrom_c_h_second[2];	/*hundredths of second*/
	char	cdrom_c_timezone;	/*timezone, offset from Greenwich Mean Time 
				  in number of 15 minutes intervals from
				  -48(West) to -52(East)*/
/*modificaton time*/
	char	cdrom_m_year[4];	/*years since year 0000*/
	char	cdrom_m_month[2];	/*month*/
	char	cdrom_m_day[2];	/*day*/
	char	cdrom_m_hour[2];	/*hour*/
	char	cdrom_m_minute[2];	/*minute*/
	char	cdrom_m_second[2];	/*second*/
	char	cdrom_m_h_second[2];	/*hundredths of second*/
	char	cdrom_m_timezone;	/*timezone, offset from Greenwich Mean Time 
				  in number of 15 minutes intervals from
				  -48(West) to -52(East)*/
/*expiration time*/
	char	cdrom_x_year[4];	/*years since year 0000*/
	char	cdrom_x_month[2];	/*month*/
	char	cdrom_x_day[2];	/*day*/
	char	cdrom_x_hour[2];	/*hour*/
	char	cdrom_x_minute[2];	/*minute*/
	char	cdrom_x_second[2];	/*second*/
	char	cdrom_x_h_second[2];	/*hundredths of second*/
	char	cdrom_x_timezone;	/*timezone, offset from Greenwich Mean Time 
				  in number of 15 minutes intervals from
				  -48(West) to -52(East)*/
/*effective time*/
	char	cdrom_e_year[4];	/*years since year 0000*/
	char	cdrom_e_month[2];	/*month*/
	char	cdrom_e_day[2];	/*day*/
	char	cdrom_e_hour[2];	/*hour*/
	char	cdrom_e_minute[2];	/*minute*/
	char	cdrom_e_second[2];	/*second*/
	char	cdrom_e_h_second[2];	/*hundredths of second*/
	char	cdrom_e_timezone;	/*timezone, offset from Greenwich Mean Time 
				  in number of 15 minutes intervals from
				  -48(West) to -52(East)*/
	u_char	cdfs_fs_version; /*file sturcture version:1 for ISO-9660*/
	char	cdrom_unused4;
	char	cdrom_appl_use[APPL_USE_SIZ]; /*reserved for application*/
	char	cdrom_future_use[653]; /*reserved for future. Note that if
					total size of field before this one
					is changed, this size should be 
					changed so that the size of this
					structure is 2048*/
};

struct	hcddfs			/*primary volume descriptor-HSG*/
{
	u_int cdrom_loc_lsb;	/*login block number (LSB) of this descriptor*/
	u_int cdrom_loc_msb;	/*login block number (MSB) of this descriptor*/
	char cdrom_vold_type;	/*volume descriptor type*/
	char cdrom_std_id[STD_ID_SIZ];	/*id "CDROM" for HSG */
	char cdrom_vold_version;	/*should be 1 for HSG */
	char cdrom_unused1;	/*spare*/
	char cdrom_sys_id[SYS_ID_SIZ];	/*id of a system that knows contents of 
				  system area, logic sector 0-15*/
	char cdrom_vol_id[VOL_ID_SIZ];	/*id of this volume*/
	char cdrom_unused2[8];	/*spare*/
	int  cdrom_vol_size_lsb;	/*size (LSB) of volume in logic block*/
	int  cdrom_vol_size_msb;	/*size (MSB) of volume in logic block*/
	char cdrom_unused3[32];	/*spare*/
	ushort cdrom_volset_siz_lsb; /*size (LSB) of volume set*/
	ushort cdrom_volset_siz_msb; /*size (MSB) of volume set*/
	ushort cdrom_volset_seq_lsb; /*sequence number (LSB) of volume in the set*/
	ushort cdrom_volset_seq_msb; /*sequence number (MSB) of volume in the set*/
	ushort cdrom_logblk_siz_lsb; /*size of (LSB) of logic block in bytes*/
	ushort cdrom_logblk_siz_msb; /*size of (MSB) of logic block in bytes*/
	u_int  cdrom_pathtbl_siz_lsb; /*size (LSB) of path table in bytes */
	u_int  cdrom_pathtbl_siz_msb; /*size (MSB) of path table in bytes */
	u_int  cdrom_pathtbl_loc_lsb; /*logical block number(LSB) of path table*/
	u_int  cdrom_pathtblo1_loc_lsb; /*logical block number(LSB) of 
					optional path table*/
	u_int  cdrom_pathtblo2_loc_lsb; /*logical block number(LSB) of 
					optional path table*/
	u_int  cdrom_pathtblo3_loc_lsb; /*logical block number(LSB) of 
					optional path table*/
	u_int  cdrom_pathtbl_loc_msb; /*logic block num.(MSB) of path table*/
	u_int  cdrom_pathtblo1_loc_msb; /*logic block num(MSB) of optional 
				      path table*/
	u_int  cdrom_pathtblo2_loc_msb; /*logic block num(MSB) of optional 
				      path table*/
	u_int  cdrom_pathtblo3_loc_msb; /*logic block num(MSB) of optional 
				      path table*/
	struct min_cddir	cdrom_rootdp; /*directory record of root*/
	char	cdrom_vol_set_id[VOL_SET_ID_SIZ];	/*id of the volume set*/
	char	cdrom_pb_id[PUBLISHER_ID_SIZ];	/*publisher's id*/
	char	cdrom_pp_id[PREPARER_ID_SIZ];	/*preparer's id*/
	char	cdrom_ap_id[APPLICATION_ID_SIZ];	/*application id*/
	char	cdrom_copyright[CDMAXNAMLEN-5];	/*copyright in this file under
						  root directory, the max. 
						  len. is 12, the rest unused*/
	char	cdrom_abstract[CDMAXNAMLEN-5];	/*abstract in this file under
						  root directory, the max. 
						  len. is 12, the rest unused*/
/*the nest four chunks are creation time, modificaton time, expiration time 
  and effective time.  Since the date/time info uses 17 bytes(odd), I can't
  use a structure (compiler round up to even bytes).  If for any reason this
  info is changed, make sure to fix all four of them.*/
/*creation time*/
	char	cdrom_c_year[4];	/*years since year 0000*/
	char	cdrom_c_month[2];	/*month*/
	char	cdrom_c_day[2];	/*day*/
	char	cdrom_c_hour[2];	/*hour*/
	char	cdrom_c_minute[2];	/*minute*/
	char	cdrom_c_second[2];	/*second*/
	char	cdrom_c_h_second[2];	/*hundredths of second*/
/*modificaton time*/
	char	cdrom_m_year[4];	/*years since year 0000*/
	char	cdrom_m_month[2];	/*month*/
	char	cdrom_m_day[2];	/*day*/
	char	cdrom_m_hour[2];	/*hour*/
	char	cdrom_m_minute[2];	/*minute*/
	char	cdrom_m_second[2];	/*second*/
	char	cdrom_m_h_second[2];	/*hundredths of second*/
/*expiration time*/
	char	cdrom_x_year[4];	/*years since year 0000*/
	char	cdrom_x_month[2];	/*month*/
	char	cdrom_x_day[2];	/*day*/
	char	cdrom_x_hour[2];	/*hour*/
	char	cdrom_x_minute[2];	/*minute*/
	char	cdrom_x_second[2];	/*second*/
	char	cdrom_x_h_second[2];	/*hundredths of second*/
/*effective time*/
	char	cdrom_e_year[4];	/*years since year 0000*/
	char	cdrom_e_month[2];	/*month*/
	char	cdrom_e_day[2];	/*day*/
	char	cdrom_e_hour[2];	/*hour*/
	char	cdrom_e_minute[2];	/*minute*/
	char	cdrom_e_second[2];	/*second*/
	char	cdrom_e_h_second[2];	/*hundredths of second*/


	u_char	cdfs_fs_version; /*file sturcture version:1 for ISO-9660*/
	char	cdrom_unused4;
	char	cdrom_appl_use[APPL_USE_SIZ]; /*reserved for application*/
	char	cdrom_future_use[680]; /*reserved for future. Note that if
					total size of field before this one
					is changed, this size should be 
					changed so that the size of this
					structure is 2048*/
};
struct	cdfs
{
/*	struct	cdfs *cdfs_link;		/* linked list of file systems */
/*	struct	cdfs *cdfs_rlink;		/*     used for incore super blocks */
	u_int	cdfs_type;		/* volume type */
	u_int	cdfs_magic;		/* HSG_FORMAT or ISO-FORMAT fs */
	u_int	cdfs_set_size;		/* volume set size */
	u_int	cdfs_seq_num;		/* sequence number of the disk currently
					   accessable */
	u_int	cdfs_pathtbl_siz;	/* the actual size (in bytes) of 
					   path table*/
	cdno_t	cdfs_rootcdno;		/* cdnumber for root directory*/
	daddr_t	cdfs_sblkno;		/* addr of super-block in filesys */
	daddr_t	cdfs_pathtblno;		/* offset of path table in filesys */
	struct min_cddir	cdfs_rootdp;	/*directory record of root*/
#ifdef __hp9000s300
	char	cdfs_pad[2];
#endif
	long	cdfs_size;		/* number of blocks in cdfs */
	long	cdfs_sec_size;		/* size of basic sectors in cdfs */
	long	cdfs_lbsize;		/* size (in bytes) of logic blocks 
					   in cdfs */
	long	cdfs_lb;			/* number of logic block in a 
					   sector in cdfs */
/* these fields can be computed from the others */
	long	cdfs_lbtolsc;		/* for "lsbtolb" and "lbtolsb" */
	long	cdfs_lstodb;		/* for "lbtodb" and "dbtolb" */
	long	cdfs_scmask;		/* ``kscoff'' calc of blk offset */
	long	cdfs_lbmask;		/* ``klboff'' calc of frag offset */
	long	cdfs_lsshift;		/* ``lblkno'' calc of logical blkno */
	long	cdfs_lbshift;		/* ``numfrags'' calc number of frags */
	
	long	cdfs_id[2];		/* file system id */
	long	cdfs_sparecon[4];	/* reserved for future constants */
/* these fields are cleared at mount time */
	char   	cdfs_flags;   		/* currently unused flag */
	char	cdfs_cdfsmnt[MAXMNTLEN];	/* name mounted on */
	char	cdfs_vol_id[VOL_ID_SIZ];	/*id of this volume*/
	char	cdfs_vol_set_id[VOL_SET_ID_SIZ];	/*id of the volume set*/
	char	cdfs_copyright[CDMAXNAMLEN];	/*copyright in this file under
						  root directory, the max. 
						  len. is 18, the rest unused*/
	char	cdfs_abstract[CDMAXNAMLEN];	/*abstract in this file under
						  root directory, the max. 
						  len. is 18, the rest unused*/
	char	cdfs_bibliographic[CDMAXNAMLEN];	/*bibliographic in this file 
						  under root directory, the max.
						  len. is 18, the rest unused*/
	long	cdfs_gen;		/*a unique number for a file system*/
	int	cdfs_spare[10];		/* reserved for future use*/
};
/*fsctl commands*/
#define CDFS_DIR_REC	1
#define CDFS_XAR	2
#define CDFS_AFID	3
#define CDFS_BFID	4
#define CDFS_CFID	5
#define CDFS_VOL_ID	6
#define CDFS_VOL_SET_ID	7
#define CDFS_CONV_CASE  8
#define CDFS_ZAP_VERS   9
#define CDFS_ZERO_FLAGS 10
/* flags  */
#define C_CONVERT_CASE  0x01
#define C_ZAP_VERSION   0x02
#define C_ZERO_FLAGS    0x04

#define cdlbno(cdfs, loc)		/* calculates (loc / cdfs->cdfs_lbsize) */ \
	((loc) >> (cdfs)->cdfs_lbshift)
#define cdlsno(cdfs, loc)	/* calculates (loc / cdfs->cdfs_sec_size) */ \
	((loc) >> (cdfs)->cdfs_lsshift)

#define cdlbtooff(cdfs, lbn)	((lbn) << (cdfs)->cdfs_lbshift)
#define cdsctooff(cdfs, sc)	((sc) << (cdfs)->cdfs_lsshift)

/* round up to file system logic block */
#define lblkroundup(cdfs, size) \
		(((size) + (cdfs)->cdfs_lbsize - 1) & ~(cdfs)->cdfs_lbmask)
/* round up to file system sector */
#define scroundup(cdfs, size) \
        (((size) + (cdfs)->cdfs_sec_size - 1) & ~(cdfs->cdfs_scmask))

/* offset of file system logic block */
#define cdlboff(cdfs, loc)	((loc) & ~(cdfs)->cdfs_lbmask)
/* offset of file system logic sector */
#define cdlsoff(cdfs, loc)	((loc) & ~(cdfs)->cdfs_scmask)

/* file system logic sector and logic block conversion */
#define lsbtolb(cdfs, b)  ((b) << (cdfs)->cdfs_lbtolsc)
#define lbtolsb(cdfs, b)  ((b) >>(cdfs)->cdfs_lbtolsc)

/* file system logic sector and DEV_BSIZE block conversion */
#define lstodb(cdfs, b)  ((b) << (cdfs)->cdfs_lstodb)
#define dbtols(cdfs, b)  ((b) >> (cdfs)->cdfs_lstodb)

#define	HSGFS	0		/* HSG file system */
#define	ISO9660FS	1	/* ISO-9660 file system */

#endif /* _SYS_CDFS_INCLUDED */
