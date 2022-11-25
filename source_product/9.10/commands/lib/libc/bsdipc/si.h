/************************************************************************/
/*								
/*			       si.h		
/*						
/*  Author:  Mike Robinson		
/*				
/*  Date Begun: 2-29-84	
/*
/*  Module: 		si.h
/*  Project:  		
/*  Revision:		3.2
/*  SCCS Name:		/users/fh/hacksaw/sccs/si/header/s.si.h
/*  Last Delta Date:	86/06/09
/*
/*  This header file contains all constants, macros and data structure
/*  defineations that an si user would need.  This file should be 
/*  included by all programs that utilize si.
/*
/************************************************************************/

/** SI constants **/

#define		SI_NO_FLAGS	         0
#define		SI_REQ_RPLY_BIT		 0x40000000    /* save in state.flags */
#define		SI_BUF_KERNEL_BIT	 0x20000000
#define		SI_ENABLE_ASYNC_BIT	 0x10000000
#define		SI_CONCAT_BIT		 0x8000000
#define 	SI_MORE_DATA_BIT	 0x20
#define		SI_PREVIEW_BIT		 0x2
#define		SI_RECV_BIT		 0x4000000
#define		SI_DESTROY_BIT		 0x4	     	
#define		SI_STREAM_BIT		 0x80	      /* save in state.flags */
#define		SI_BKRLY_URG_BIT	 0x80
#define		SI_ARPA_BIT		 0x40	      /* save in state.flags */
#define		SI_NO_WAIT_BIT		 0x2000000
#define		SI_INET_ADDR_BIT	 0x10	
#define		SI_SECURITY_BIT		 0x8000000    /* save in state.flags */
#define		SI_PRECEDENCE_BIT	 0x1000	      /* save in state.flags */

#define 	SI_DYNAMIC_PORT		0
#define 	SI_PARM_BLK_SIZE	100
#define		SI_MAX_NAME_LENGTH	55

/** SI CNTL TYPES **/

#define		SIDATACNT		101
#define		SIURGDATA		102
#define		SIRSELECT		103
#define		SIWSELECT		104
#define		SILSELECT		105
#define		SICSELECT		106
#define		SISETSIG		107
#define		SIRETSIG		108
#define		SIKEEPALIVE		109
#define		SITIMEOUT		110
#define		SI_RECV_THRESHOLD	111

/** SI typedefs **/


/************************************************************************
*
*		I  O  D	 E  S  C  R  _	T  Y  P	 E
*
* Purpose	The purpose of this  structure is to give enough info to
*		iowait so that it can  determine the io request the user
*		is asking  about.  This	 structure is copied back to the
*		user when he initiates	and IO operation.  The user then
*		passes it back to SI when he does an iowait.
*
* Allocation	The SI user allocates this structure within its code.	
*
* Field_Descriptions							
*
* cd		This is the index into the si_global->cd.  It references
*		a csr.
*
* seq_num	This is kind of a validation  check.  This is an integer
*		value that should  match up with  in_msg.iodescr_num  or
*		out_msg.iodescr_num.   It   validates	 the	iodescr.
*		Currently  there is no	validation of iodescr's if it is
*		an  iowait on a connect	 or  recvcn.  This  could  cause
*		problems later on.
*
* concat	True if the user did a prior  send with the  concat  bit
*		set.  Only  looked at if this  iodescr is being	 used in
*		conjunction with a send.
*
* msg		This is an index into the correct  message  carrier that
*		is being waited for.  Used for both  inbound and oubound
*		activity.
*
* flags		Flags that can be passed from the IO initiation routine
*		to the iowait routine.
*
************************************************************************/

typedef struct {
	word		intrinsic;
	unsdword	cd;
	unsdword	seq_num;
	boolean		concat;
	word		msg;
	unsdword	flags;
	} iodescr_type;

typedef  dword (*sel_proc)();

typedef struct {
	unsdword	ident;
	sel_proc	ack;
	} si_select_parm;

typedef struct {
	unsdword	linet;
	unsdword	lport;
	unsdword	rinet;
	unsdword	rport;
	} si_inet_addr;

typedef struct {
	byte		precedence;
	char		security[11];
} si_sec_prec;

/* ------------------------ end of si.h ------------------------------------ */
