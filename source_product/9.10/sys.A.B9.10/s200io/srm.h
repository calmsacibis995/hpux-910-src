/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/srm.h,v $
 * $Revision: 1.4.84.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 16:42:10 $
 */
/* HPUX_ID: @(#) $Revision: 1.4.84.4 $  */
#ifndef _MACHINE_SRM_INCLUDED /* allows multiple inclusion */
#define _MACHINE_SRM_INCLUDED
#ifdef __hp9000s300

#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

/* srm ioctl command defines */
#define	SRMLOCK		_IO('G',1)
#define	SRMUNLOCK	_IO('G',2)

/* the next define is for a request to change the timeout value
 * for the srm 98629 card. The user will need to pass a third 
 * argument to the ioctl call in the arg parameter. The type should 
 * be integer and the units are in 10ths of a second. eg 600=60 secs.
*/
#define SRMCHGTMO	_IO('G',3)
#define SRMSETCTL       _IO('G',4)
#define SRMGETNODE      _IOR('G',5,int)
#define SRMRESET        _IO('G',6)
#define SRMSETQSIZE     _IO('G',7)
#define SRMGETQSIZE     _IOR('G',8,int)


#ifdef _KERNEL

#ifdef SRM_MEASURE

struct srmstat {
    int sem_try_cnt;
    int sem_fail_cnt;
    int max_rcvqsize;
    int rcv_bufovf_cnt;
    int rcv_crc_cnt;
    int rcv_fill_cnt;
    int tx_retry_cnt;
    int int_cnt;
    int pkt_cnt;
    int max_int_packs;
    int max_int_time;
    int datahold_cnt;
    int error_cnt;
    int last_error;
};

#define SRMSTATS        _IOR('G',99,struct srmstat)

#endif

#define MAX_SRM_PACK_SIZE       767 /* 98629 won't allow larger  */
#define MIN_SRM_PACK_SIZE       5   /* 98629 won't allow smaller */
#define SRM_DEF_BUF_SIZE        4096

#define SRM_VERSION             0x50
#define SRM_OTHER		7
#define SRM_MODEL               300

#define SRM_GETNODE             0x86
#define SRM_SETINTR             0x79
#define SRM_SETRETRY            0x0c

#ifdef SRM_MEASURE
#define SRM_CRC                 0x87
#define SRM_OVF                 0x88
#define SRM_TXRETRY             0x8c
#endif


#define SRM_INT_MASK            0x03
#define SRM_INT_LVL             3

#define SRM_POINTER(base,ptr_addr) ((unsigned char *)(base) + ( (*((ptr_addr) + 2) << 9) | (*(ptr_addr) << 1) | 0x1))
#define SRM_DATA(ptr_addr) ( (*((ptr_addr) + 2) << 8) | *(ptr_addr) )

#define DSDP_P0_OFFSET 0x000006
#define TXBUFF         0x000004
#define RXBUFF         0x000024
#define CTRL_AREA      0x000000
#define DATA_AREA      0x000010
#define Q_ADDR         0x000000
#define Q_SIZE         0x000004
#define Q_FILL         0x000008
#define Q_EMPTY        0x00000C

/* srm flags */

#define SRM_RCOLL       0x01
#define SRM_RCVTRG      0x02
#define SRM_DATAHOLD    0x04
#define SRM_RDSLEEP     0x08
#define SRM_TIMEOUT     0x10

/* Interrupt Conditions */

#define SRMINT_ERROR   0x01
#define SRMINT_RCVDATA 0x02

struct srm_table {
    struct srm_reg *srm_regs; /* Pointer to card registers */
    int     srm_timeout;    /* timeout value */
#ifdef _CLASSIC_ID_TYPES
    u_short srm_filler_pid;
    short   srm_pid;        /* pid of locking process */
#else
    pid_t   srm_pid;        /* pid of locking process */
#endif
    int     srm_revision;   /* If srm, contains revision */
    int     srm_node;       /* Node number */
    struct proc *srm_rsel;  /* proc pointer for first proc to do select */
    struct srm_table *srm_next_dev;
    int     srm_rcv_packets;
    int     srm_partial_read;
    int     srm_rcv_bufsize;
    u_char  *srm_rcv_buffer;
    u_char  *srm_rcv_buftail;
    u_char  *srm_rcv_bufhead;
    int     tx_data_qsize;
    u_char  *tx_data_queue;
    u_char  *tx_data_qempty_ptr;
    u_char  *tx_data_qfill_ptr;
    int     tx_ctrl_qsize;
    u_char  *tx_ctrl_queue;
    u_char  *tx_ctrl_qempty_ptr;
    u_char  *tx_ctrl_qfill_ptr;
    int     rx_data_qsize;
    u_char  *rx_data_queue;
    u_char  *rx_data_qempty_ptr;
    u_char  *rx_data_qfill_ptr;
    int     rx_ctrl_qsize;
    u_char  *rx_ctrl_queue;
    u_char  *rx_ctrl_qempty_ptr;
    u_char  *rx_ctrl_qfill_ptr;
#ifdef SRM_MEASURE
    int sem_try_cnt;
    int sem_fail_cnt;
    int max_rcvqsize;
    int rcv_fill_cnt;
    int int_cnt;
    int pkt_cnt;
    int max_int_packs;
    int max_int_time;
    int datahold_cnt;
    int error_cnt;
    int last_error;
#endif
    struct sw_intloc rcv_intloc;
    u_char  srm_sc;
    u_char  srm_flags;
    u_char  srm_tx_buffer[MAX_SRM_PACK_SIZE+1]; /* +1 is just to align */
};

struct srm_reg {
	u_char filler1;
	u_char id_reset;
	u_char filler2;
	u_char interrupt;
	u_char filler3;
	u_char semaphore;
	u_char filler4[0x3ffb];
	u_char int_cond;
	u_char filler5;
	u_char command;
	u_char filler6;
	u_char data;
	u_char filler7;
	u_char primary_addr;
	u_char filler8;
	u_char dsdp_low;
	u_char filler9;
	u_char dsdp_high;
	u_char filler10;
	u_char error_code;
	u_char filler11[0x4f];
	u_char is_srm;
	u_char filler12;
	u_char retries;
	u_char filler13;
	u_char version;
	u_char filler14;
	u_char model;
};

#endif /* _KERNEL */
#endif /* __hp9000s300 */
#endif /* _MACHINE_SRM_INCLUDED */
