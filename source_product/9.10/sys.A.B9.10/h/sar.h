/*
 * @(#)sar.h: $Revision: 1.11.83.4 $ $Date: 93/09/17 18:33:32 $
 * $Locker:  $
 */

/* This header file is for SAR-specific    declarations          */

extern  long 	mux0incnt;
extern  long	mux0outcnt;
extern  long 	mux2incnt;
extern  long	mux2outcnt;
extern  long	ptyincnt;
extern  long	ptyoutcnt;
extern  long	hptt0cancnt;
extern  long	hptt0rawcnt;
extern  long	hptt0outcnt;
extern  long    sysread;
extern  long    syswrite;
extern  long	msgcnt;
extern  long	semacnt;
extern  long    runque;
extern  long    runocc;
extern  long    swpque;
extern  long    swpocc;
    
/*  This structure is used to determine when the cpu is idle and waiting
	for io.                                                         */
            
struct syswait {
	short iowait;
	short swap;
	short physio;
};
extern struct syswait syswait;

extern	 long readdisk;
extern   long writedisk;
extern   long phread;
extern   long phwrite;
extern   long lwrite;
extern   long sysexec;
extern   long sysnami;
extern   long sysiget;
extern   long dirblk;
extern   long inodeovf;
extern   long fileovf;
extern   long textovf;
extern   long procovf;
extern   struct timeval dk_resp[];
extern 	 struct timeval dk_resp1[];
extern   struct timeval dk_resp2[];
extern   struct timeval dk_resp3[];

extern long sar_swapin;
extern long sar_swapout;
extern long sar_bswapin;
extern long sar_bswapout;
