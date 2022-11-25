
/* Nlist structure of symbols */





#define	X_PROC 0
#define	X_FIRSTFREE 1
#define	X_MAXFREE 2
#define	X_TEXT 3
#define	X_FREEMEM 4
#define	X_NPROC 5
#define	X_NTEXT 6
/* 		     #define	UNUSED 7 */
#define	X_BSWLIST 8
#define	X_NSWBUF 9
#define	X_SWBUF 10
#define	X_BUF 11	
#define	X_NBUF 12
#define	X_BUFHASH 13
#define	X_BFREELIST 14
#define	X_INODE 15
#define	X_MOUNT 16
#define	X_NINODE 17
#define	X_IHEAD 18
#define	X_NSHMEM 19
#define	X_NSHMINFO 20

#define	X_NSHM_SHMEM 21
#define	X_PDIR 22
#define	X_HTBL 23
#define	X_IOPDIR 24
#define	X_NPDIR 25
#define	X_NHTBL 26
#define	X_NIOPDIR 27
#define	X_NPIDS 28
#define	X_NIOPIDS 29
#define	X_IFREEH 30

#define	X_IFREET 31
#define	X_FILE 32
#define	X_NFILE 33
#define	X_LOTSFREE 34
#define	X_MINFREE 35
#define	X_DESFREE 36
#define	X_VERSION 37
#define	X_CAMAP 38
#define	X_ECAMAP 39
#define	X_SHM_VFDS 40

#define	X_QS 41
#define	X_SWAPDEV 42
#define	X_END 43
#define	X_ETEXT 44
#define	X_RPB 45
/* REGION STUFF */
#define	X_PHEAD 46
#define	X_PBAD 47
#define	X_PFDAT 48
#define	X_PHASH 49
#define	X_PHASHMASK 50

#define	X_PREGFREE 51
#define	X_PACTIVE 52
#define	X_REGION 53
#define	X_REGACTIVE 54
#define	X_REGFREE 55
#define	X_PTFREE 56
#define	X_SWAPTAB 57
#define	X_NEXTSWAP 58
#define	X_SYSMAP 59
#define	X_TUNE 60

#define	X_SYSINFO 61
#define	X_V 62
#define	X_PHYSMEM 63
#define	X_SWAPWANT 64
#define	X_PSEGS 65
#define	X_SLROOT 66
#define X_PREGION 67
/* Vnode stuff */
#define X_ROOTVFS 68
#define X_UFS_VFSOPS 69
#define X_UFS_VNODEOPS 70

#define X_NFS_VFSOPS 71
#define X_NFS_VNODEOPS 72
#define X_RTABLE 73
#define X_RPFREELIST 74
#define X_UIDHASH 75
#define X_NCACHE 76
#define X_NC_HASH 77
#define X_NC_LRU 78
#define X_NCSTATS 79

/*			80   use to be X_DEVICES */
/*			81   used to be X_NDEVICES */

#define X_NSP      	82
#define X_NCSP     	83
#define X_MAX_NSP  	84
#define X_MOUNTLOCK	85
#define X_MOUNT_WAITING	86
#define X_SELFTEST	87
#define X_CLUSTAB	88
#define X_MY_SITE	89
#define X_ROOT_SITE	90

#define X_SWAP_SITE	91
#define X_MY_SITE_STATUS	92
#define X_USING_ARRAY	93
#define X_USING_ARRAY_SIZE   94
#define X_SERVING_ARRAY 95
#define X_SERVING_ARRAY_SIZE 96
/* 			97   used to be X_DSKLESS_MBUFS */
/* 			98   used to be X_DSKLESS_CBUFS */
#define X_DUX_MFREE	99
#define X_DBC_HDR	100

#define X_DUX_MBSTAT	101
#define X_NET_BCHAIN 	102  /* used to be X_MBUF_XSTATS */
#define X_IN_PROT_STATS	103
#define X_OUT_PROT_STATS 104
#define X_PHYSMEMBASE 105
#define X_MPPROCINFO 106
#define X_SPL_LOCK 107
#define X_SPL_WORD 108
#define X_STACKADDR 109
#define	X_VASTABLE 110
#define X_GLOBAL_LOCK_LIST 111
#define X_GLOBAL_SEMA_LIST 112
#define X_SEMAPHORE_LOG 113

#define X_TRFREEQ		114
#define X_TRUNCOMMITTEDQ	115
#define X_TRCOMMITTEDQ		116

#define X_SYSSEGTAB  117
#define X_UBASE      118
#define X_CURPROC    119

#define X_SWDEVT     120
#define X_SWAPDEV_VP 121

#define X_CRASH_P_TABLE	122
#define X_CRASH_E_TABLE	123
#define X_PANIC_RPB	124
#define X_PANIC2_RPB	125
#define X_BOOTTIME	126
#define	X_BUFHSZ	145
#define	X_INOHSZ	146
#define	X_UIDHMASK	147
#define X_KERNVAS	148
#define X_TT_REGION_ADDR	149
#define X_PGTOPDE_TABLE	150
#define X_SCALED_NPDIR	151
#define X_PDIRHASH_TYPE	152
#define X_KMEMSTATS	153
#define X_SHMMNI	154
#define X_NCLIST	155
#define X_BUFPAGES	156
#define X_CFREE		157
#define X_THREE_LEVEL_TABLES  158
#define X_PST_CMDS	159
#define X_DQHEAD	160
#define X_DQFREE	161
#define X_DQRESV	162
