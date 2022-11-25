/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sio/RCS/autoch.c,v $
 * $Revision: 1.13.83.5 $    $Author: marshall $
 * $State: Exp $        $Locker:  $
 * $Date: 93/12/09 14:32:19 $
 */

/* HPUX_ID: %W%         %E% */

/*
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

#include "../h/stdsyms.h"       /* get _WSIO defined appropriately */


#ifdef _WSIO
#define PASS_THRU 
#endif

#include "../h/ioctl.h"
#include "../h/errno.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../h/sysmacros.h"
#include "../h/types.h"
#include "../h/time.h"
#include "../h/systm.h"
#include "../h/conf.h"

#include "../h/user.h"
#include "../h/proc.h"
#include "../h/uio.h"

#include "../h/vnode.h"
#include "../h/acdl.h"

#ifndef _WSIO
#include "../h/file.h"
#include "../sio/iotree.h"
#include "../sio/llio.h"
#include "../sio/scsi_meta.h"
#include "../h/diskio.h"
#include "../h/scsi.h"
#include "../ufs/inode.h"
#include "../sio/autox0.h"
#include "../sio/autoch.h"
#include "../h/kern_sem.h"
#endif

#ifdef __hp9000s700
#include "../h/file.h"
#include "../h/scsi.h"
#include "../h/diskio.h"
#include "../sio/autoch.h"
#include "../wsio/autox.h"
#include "../h/io.h"
#include "../wsio/scsi_ctl.h"
#undef timeout
#endif

#ifdef __hp9000s300
#include "../h/file.h"
#include "../h/scsi.h"
#include "../h/diskio.h"
#include "../sio/autoch.h"
#include "../s200io/autox.h"
#endif  

#include "../h/ioctl.h"
/* include for referencing the process Id */
#include "../h/resource.h"
#include "../h/proc.h"
#include "../h/ki_calls.h"

/* controls what diagnostics are printed */
#ifndef _WSIO
int AC_DEBUG =  0; /* 0x7b3f;  */
#endif
#ifdef __hp9000s700
int AC_DEBUG =  0; /* 0x7b3f; */
#endif
#ifdef __hp9000s300
int AC_DEBUG =  0; /* 0x7b3f; */
#endif

/********************************************
**********  Global Variables ****************
*********************************************/

/* block major number of the drive in the autochanger */
int drive_maj_blk = SCSI_MAJ_BLK;

/* char  major number of the drive in the autochanger */
int drive_maj_char = SCSI_MAJ_CHAR;

int ac_open_init = 0;

/*
 * The head of a doubly linked list of autochangers
 */

struct ac_device *ac_device_head = NULL;
struct dev_store *dev_store_head = NULL;

/********************************************
**********         Macros         ***********
*********************************************/

/* in order to use these 2 macros,                   */
/* you must have 'sreqp' and 'ac_device_ptr' defined */
#define FOR_EACH_SWAP_REQUEST \
   {int priority; \
      /* keep out all interrupts that might modify the list */ \
      priority = spl6(); \
      /* loop thru the queue */ \
      sreqp = ac_device_ptr->sreqq_head; \
      if (sreqp != NULL) { \
      do {
#define END_FOR \
         sreqp = sreqp->forw; \
         } while (sreqp != ac_device_ptr->sreqq_head); \
      } \
      splx(priority); \
   }
#define RETURN_FROM_FOR(return_value) {splx(priority); return(return_value);}



/*
 * This macro adds a structure to a doublely linked list 
 * at the end of the list.  
 */

#define ADD_TO_LIST(head,struct_ptr) \
   {int priority; \
      /* keep out all interrupts that might modify the list */ \
      priority = spl6(); \
      if (head == NULL) {         /* list is empty     */ \
         head   = struct_ptr; \
         struct_ptr->back  = struct_ptr; \
         struct_ptr->forw  = struct_ptr; \
      } else {             /* list is not empty */ \
         head->back->forw = struct_ptr; \
         struct_ptr->back = head->back; \
         head->back = struct_ptr; \
         struct_ptr->forw = head; \
      } \
      splx(priority); \
   }


/*
 * This macro adds a structure to a doublely linked list 
 * at the head of the list.  
 */

#define ADD_TO_HEAD_OF_LIST(head,struct_ptr) \
   {int priority; \
      /* keep out all interrupts that might modify the list */ \
      priority = spl6(); \
      if (head == NULL) {         /* list is empty     */ \
         head   = struct_ptr; \
         struct_ptr->back  = struct_ptr; \
         struct_ptr->forw  = struct_ptr; \
      } else {             /* list is not empty */ \
         struct_ptr->forw = head; \
         struct_ptr->back = head->back; \
         head->back = struct_ptr; \
         struct_ptr->back->forw = struct_ptr; \
         head  = struct_ptr; \
      } \
      splx(priority); \
   }

#define DELETE_FROM_LIST(head,struct_ptr) \
   {int priority; \
      /* keep out all interrupts that might modify the list */ \
      priority = spl6(); \
      if (struct_ptr->forw == struct_ptr) {  \
          /* only element in the list  */ \
         head = NULL; \
      } else { \
         struct_ptr->back->forw = struct_ptr->forw; \
         struct_ptr->forw->back = struct_ptr->back; \
         if (head == struct_ptr)      \
            /* first element on list  */ \
            head = struct_ptr->forw; \
      } \
      splx(priority); \
   }

/* Only valid for block devices.  Debug only */
#define CHECK_DELETE(struct_ptr) \
   if ((struct_ptr->bp->b_dev & 0xffff) == 0) \
      panic("Deleted without doing.");

/* kmem_free_intr does not exist on 800 */

#define FREE_ENTRY(struct_ptr,struct_type) \
   /* free up the memory from the deleted request */ \
   kmem_free ((caddr_t)struct_ptr,(u_int)(sizeof(struct struct_type))); 
   /*kmem_free_intr ((caddr_t)struct_ptr,(u_int)(sizeof(struct struct_type))); */

/*** external definition for autox interface calls ***/
extern int autox0_ioctl();
extern int autox0_open();

int
ac_noop()
{
   /* just a placeholder function */ return(0);
}

ac_putchar(c)
   register c;
{
   if (c==0 || c=='\r' || c==0177)    /* useless stuff? */
      return;          /* forget it */

   /* Initialize buffer if needed */
   if (ac_msgbuf.msg_magic != AC_MSG_MAGIC) {
      register int i;

      ac_msgbuf.msg_bufx = 0;
      ac_msgbuf.msg_magic = AC_MSG_MAGIC;
      for (i=0; i < AC_MSG_BSIZE; i++)
         ac_msgbuf.msg_bufc[i] = 0;
   }

   if (ac_msgbuf.msg_bufx < 0 || ac_msgbuf.msg_bufx >= AC_MSG_BSIZE)
      ac_msgbuf.msg_bufx = 0;

   ac_msgbuf.msg_bufc[ac_msgbuf.msg_bufx++] = c;
}

ac_copybuf(s)
   char *s;
{
   int i=0;
   while (s[i] != '\0') {
      ac_putchar(s[i]);
      i++;
   }
}

msg_printf0(fmt)
   char *fmt;
{
   char tmp_buf[120];
   int len=120;
   
   sprintf(tmp_buf,len,fmt);
   ac_copybuf(tmp_buf);
}

msg_printf1(fmt,x1)
   char *fmt;
   unsigned x1;
{
   char tmp_buf[120];
   int len=120;
   
   sprintf(tmp_buf,len,fmt,x1);
   ac_copybuf(tmp_buf);
}

msg_printf2(fmt,x1,x2)
   char *fmt;
   unsigned x1,x2;
{
   char tmp_buf[120];
   int len=120;
   
   sprintf(tmp_buf,len,fmt,x1,x2);
   ac_copybuf(tmp_buf);
}

msg_printf3(fmt,x1,x2,x3)
   char *fmt;
   unsigned x1,x2,x3;
{
   char tmp_buf[120];
   int len=120;
   
   sprintf(tmp_buf,len,fmt,x1,x2,x3);
   ac_copybuf(tmp_buf);
}

ac_mark()

{
   msg_printf0("---------------");
}

   /* Decide if you need to sleep before going on.
    * This choice is based on four variables.
    *
    * other_sur_in_q_var: true if there is another surface other than the
    *           current surface that is in the queue.
    *           (bit 3)
    *
    * current_sur_inserted_var: true if the current surface is 
    *                inserted in any drive.
    *                (bit 2)
    *
    * hog_time_ok_var: true if the length of time that the current surface
    *        has been servicing requests is less than hog time.
    *       (bit 1)
    *
    * drives_busy_var: true if all drives are busy or if a drive with the 
    *        current surface is busy.
    *        (bit 0)
    *
    * The four variables are combined to form a number 0-15.
    * The following case statement shows the possibilities for each
    * combination of the four variables.
    * 
    */



/*
 * This function returns the number of drives that are not in WAIT_TIME.
 */
int
num_drives_not_waiting(ac_device_ptr)
   struct ac_device *ac_device_ptr;
{
   int i,sum=0;
   for (i=0; i<ac_device_ptr->number_drives; i++) {
      if (ac_device_ptr->ac_drive_table[i].drive_state != WAIT_TIME) {
         sum++;
      }
   }
   return(sum);
}

/*
 * This routine decides to delay or not delay and returns the result.
 */
enum scheduling_result
calculate_scheduling_method(sreqp)
   struct surface_request *sreqp;
{
   extern struct ac_drive *get_expired_drive();
   extern struct surface_request *get_next_request();
   extern int AC_DEBUG; 
   int current_sur_inserted_var, requests_exceed_resources_var, hog_time_ok_var, 
      drives_busy_var, combination, ret_value; 
   struct ac_drive *drive_ptr,*newdrive_ptr; 
   struct surface_request *newsreqp;
   extern void wait_time_done ();
   extern void startup_request();

   /* calculate the four variables */
   requests_exceed_resources_var = requests_exceed_resources (sreqp->ac_device_ptr);

   if (current_sur_inserted_var = current_sur_inserted
                     (sreqp->ac_device_ptr, sreqp->surface, &drive_ptr)) {
               /* need to check hog time */
      hog_time_ok_var = hog_time_ok(drive_ptr);
      /* If the surface is pmounting or punmounting, return AC_FLUSH */
      if ((drive_ptr->drive_state == FLUSH) ||
          (drive_ptr->drive_state == ALLOCATED) ||
          (drive_ptr->drive_state == DAEMON_CALL)) {
         if (sreqp->r_flags & R_ASYNC) { /* only do this for async requests */
            /* drive could be in wait time.  Clean it. */
            untimeout(wait_time_done, (caddr_t)drive_ptr);
            return(AC_FLUSH);
         }
         else { 
             /* This code use to always return AC_DELAY;  There is a condition that occurs  */
             /* that this code will return AC_DELAY and the the case swith will be 0 */
             /* this causes the request on this pid to never get started: a hang on */
             /* this particular surface.  This fix is look at the surface_inproc */
             /* instead of the current surface inserted. */

             if (drive_ptr->drive_state != DAEMON_CALL) {
               return(AC_DELAY);
             }
             else {
                /* it is DAEMON_CALL which might cause a process to hang on a surface */
                /* set the variables as if the surface was not inserted and go through */
                /* the calculations to get a new drive for the surface request */
                current_sur_inserted_var = 0;
                hog_time_ok_var = 0;
             }
         }
      }
   } else {
      hog_time_ok_var = 0;    /* hog time must be false */
   }

   drives_busy_var = drives_busy(sreqp->ac_device_ptr, sreqp->surface);

   /* combine variables to generate number in range 0 - 15 */
   combination = requests_exceed_resources_var << 3 |
            current_sur_inserted_var << 2 |
            hog_time_ok_var << 1 | 
            drives_busy_var;

   if (sreqp->ac_device_ptr->transport_busy && sreqp->ac_device_ptr->jammed) {
      if (AC_DEBUG & TOP_CASE) {
         msg_printf2("%d    transport busy and JAMMED surface = %x\n",u.u_procp->p_pid,sreqp->surface);
      }
      ret_value = AC_DELAY;
   } else {
      if (AC_DEBUG & TOP_CASE) {
         msg_printf2("%d top switch for request 0x%x\n", u.u_procp->p_pid,sreqp);
         msg_printf3("%d    case = %d surface = %x\n",u.u_procp->p_pid, combination,sreqp->surface);
      }
   
      sreqp->case_switch = combination;

      /* switch on combination */
      switch (combination) {
         case  2: 
             panic("case cannot occur"); 
             break; 
         case  3: 
             panic("case cannot occur"); 
             break; 
         case 10: 
             panic("case cannot occur"); 
             break; 
         case 11: 
             panic("case cannot occur"); 
             break; 
   
         case  0: 
             ret_value = AC_NO_DELAY;
             break;
         case  4: 
             ret_value = AC_NO_DELAY;
             break;
         case  6: 
             ret_value = AC_NO_DELAY;
             break;
         case  8: 
             ret_value = AC_NO_DELAY;
             break;
         case 14: 
             ret_value = AC_NO_DELAY;
             break;
   
         case  1: 
         case  9: 
             if ((newdrive_ptr=get_expired_drive(sreqp->ac_device_ptr)) != NULL) {
               /*hogtime may have expired while drive is in waittime*/
               untimeout (wait_time_done, (caddr_t) newdrive_ptr);
               newdrive_ptr->drive_state = NOT_BUSY;
               newsreqp = get_next_request(newdrive_ptr,sreqp->ac_device_ptr);
               if (newsreqp != sreqp) {
                  if (newsreqp != NULL) {
                     /* let's do the new one and delay the current one */
                     sreqp->case_switch = 91;
                     startup_request(newsreqp,NULL);
                     ret_value = AC_DELAY;
                  } else {
                     /* get_next_request() returned null */
                     if (current_element_inserted(sreqp->ac_device_ptr,sreqp->surface,&newdrive_ptr)) {
                        /* our flip side is in use in a drive */
                        sreqp->case_switch = 92;
                        sreqp->r_flags &= ~R_BUSY;
                        ret_value = AC_DELAY;
                     } else {
                        /* let's do it! */
                        sreqp->case_switch = 93;
                        ret_value = AC_NO_DELAY;
                     }
                  }
               } else {
                  /* new request same as current request */
                  sreqp->case_switch = 94;
                  ret_value = AC_NO_DELAY;
               }
             } else {
               /* no expired drive */
               sreqp->case_switch = 95;
               ret_value = AC_DELAY;
             }
            break;
   
            break;
         case 13: 
             if ((elements_in_q(sreqp->ac_device_ptr) > num_drives_not_waiting(sreqp->ac_device_ptr)) ||
               (flip_side_in_q(sreqp->surface,sreqp->ac_device_ptr))) {
               /* more elements than drives, give up drive */
               /*       OR      */
               /* let flip side use drive */
                if (drive_ptr->drive_state == WAIT_TIME) {
                  /*hogtime has expired while drive is in waittime*/
                  /*for performance reasons, startup new request*/
                  /*now, rather than wait for wait_time to trigger*/
                  untimeout (wait_time_done, (caddr_t) drive_ptr);
                  drive_ptr->drive_state = NOT_BUSY;
            newsreqp = get_next_request(drive_ptr,sreqp->ac_device_ptr);
                  if (newsreqp != sreqp) {
                     if (newsreqp != NULL) {
                        /* let's do the new one and delay the current one */
                        sreqp->case_switch = 0xd1;
                        startup_request(newsreqp,NULL);
                        ret_value = AC_DELAY;
                     } else {
                        /* get_next_request() returned null */
                        if (current_element_inserted(sreqp->ac_device_ptr,sreqp->surface,&newdrive_ptr)) {
                           /* our flip side is in use in a drive */
                           sreqp->case_switch = 0xd2;
                           sreqp->r_flags &= ~R_BUSY;
                           ret_value = AC_DELAY;
                           panic("Autochanger: THIS CAN NEVER HAPPEN");
                        } else {
                           /* let's do it! */
                           sreqp->case_switch = 0xd3;
                           ret_value = AC_NO_DELAY;
                        }
                     }
                  } else {
                     /* new request same as current request */
                     sreqp->case_switch = 0xd4;
                     ret_value = AC_NO_DELAY;
                  }

                } else {
                  ret_value = AC_DELAY;
                }
             } else {
                /* no requests waiting to use this drive */
                /* more drives then elements in queue. if the drive is */
                /* in wait time cancel wait time and set the drive to */
                /* not busy so it can process another request */
                if (drive_ptr->drive_state == WAIT_TIME) {
                  untimeout (wait_time_done, (caddr_t) drive_ptr);
                  drive_ptr->drive_state = NOT_BUSY;
                }
               ret_value = AC_NO_DELAY;
             }
             break;
   
         case 12: 
             ret_value = AC_NO_DELAY;
             break;
   
         case  5: 
             if (drive_ptr->drive_state == WAIT_TIME) {
               untimeout (wait_time_done, (caddr_t) drive_ptr);
             }
             ret_value = AC_NO_DELAY;
             break;
         case  7: 
             if (drive_ptr->drive_state == WAIT_TIME) {
               untimeout (wait_time_done, (caddr_t) drive_ptr);
             }
             ret_value = AC_NO_DELAY;
             break;
         case 15: 
             if (drive_ptr->drive_state == WAIT_TIME) {
               untimeout (wait_time_done, (caddr_t) drive_ptr);
             }
             ret_value = AC_NO_DELAY;
             break;
         default: printf("%d    Impossible case in combination case\n", u.u_procp->p_pid);
             panic("autochanger driver bad case");
             break;
      } 
   } 

   if (AC_DEBUG & TOP_CASE) {
      msg_printf3("%d    return = %s surface = %x\n", u.u_procp->p_pid,
         (ret_value==AC_DELAY)?"DELAY   ":"NO_DELAY", sreqp->surface);
   }
   return(ret_value);
} 

/* 
 * Print queue routines
 */

void
print_surface_request(sreqp)
   struct surface_request *sreqp;

{
   printf("\t---------------------\n");
   printf("\tforw = %x\n",sreqp->forw);
   printf("\tback = %x\n",sreqp->back);
   printf("\tsurface = %d\n",sreqp->surface);
   printf("\tr_flags = %x\n",sreqp->r_flags);
   printf("\tpid = %d\n",sreqp->pid);
   printf("\tbp = %x\n",sreqp->bp);
   printf("\tac_device_ptr = %x\n",sreqp->ac_device_ptr);
   printf("\tdrive_ptr = %x\n",sreqp->drive_ptr);
   printf("\top = %x\n",sreqp->op);
   printf("\top_parmp = %x\n",sreqp->op_parmp);
   printf("\tspinup_polls = %d\n",sreqp->spinup_polls);
   printf("\tscheduling_result = %d\n",sreqp->result);
   printf("\tcase_switch = %d\n",sreqp->case_switch);
}


void 
print_surface_queue(ac_device_ptr)
   struct ac_device *ac_device_ptr;

{
   struct surface_request *sreqp;

   printf("SURFACE REQUEST QUEUE\n");
   FOR_EACH_SWAP_REQUEST
      print_surface_request(sreqp);
   END_FOR
}

void
print_xport_request(xreqp)
   struct xport_request *xreqp;
{
   printf("\t---------------------\n");
   printf("\tforw = %x\n",xreqp->forw);
   printf("\tback = %x\n",xreqp->back);
   printf("\tmove_request.transport = %d\n",xreqp->move_request.transport);
   printf("\tmove_request.source    = %d\n",xreqp->move_request.source   );
   printf("\tmove_request.destination= %d\n",xreqp->move_request.destination);
   printf("\tmove_request.invert     = %d\n",xreqp->move_request.invert     );
   printf("\tdrive_ptr = %x\n",xreqp->drive_ptr);
   printf("\tsurface_request = %x\n",xreqp->sreqp);
   printf("\tbp = %x\n",xreqp->bp);
   printf("\tx_flags = %d\n",xreqp->x_flags);
   printf("\terror   = %d\n",xreqp->error  );
   printf("\tsurface = %d\n",xreqp->surface);
   printf("\tpid = %x\n",xreqp->pid);
}

void
print_xport_queue(xreqp_head)
   struct xport_request *xreqp_head;
{
   struct xport_request *xreqp;

   xreqp = xreqp_head; 
   printf("REQUEST QUEUE\n");
   if (xreqp != NULL) { 
      do {
         print_xport_request(xreqp);
         xreqp = xreqp->forw; 
      } while (xreqp != xreqp_head); 
   } 
}

#define print_spinup_queue(head)  print_xport_queue(head) 

/*
 * This procedure creates and initializes an ac_device structure
 */
void
zero_ac_device(ac_device_ptr)
   struct ac_device *ac_device_ptr;
{
   int i,j;
   extern int initiate_deadlock_recovery();

   /* initialize the ac_device structure */
   ac_device_ptr->sreqq_head = NULL;
   ac_device_ptr->xport_queue_head = NULL;
   ac_device_ptr->spinup_queue_head = NULL;
   ac_device_ptr->opened = 0;
   for (i = 0; i < NUM_DRIVE; i++) {
      ac_device_ptr->ac_drive_table[i].ac_device_ptr = ac_device_ptr;
      ac_device_ptr->ac_drive_table[i].scsi_address = 0;
#ifndef _WSIO
      ac_device_ptr->ac_drive_table[i].logical_unit = 0;
#endif
      ac_device_ptr->ac_drive_table[i].surface = EMPTY;
      ac_device_ptr->ac_drive_table[i].surface_inproc = EMPTY;
      ac_device_ptr->ac_drive_table[i].hog_time_start = 0L;
      ac_device_ptr->ac_drive_table[i].ref_count = 0;
      ac_device_ptr->ac_drive_table[i].element_address = 0;
      ac_device_ptr->ac_drive_table[i].opened = 0;
      ac_device_ptr->ac_drive_table[i].inuse = 0;
      ac_device_ptr->ac_drive_table[i].drive_state = NOT_BUSY;
      ac_device_ptr->ac_drive_table[i].close_request = NULL;
      ac_device_ptr->ac_drive_table[i].interrupt_request = NULL;
      ac_device_ptr->ac_drive_table[i].interrupt_sreqp = NULL;
   }

   ac_device_ptr->number_drives = 0;
   ac_device_ptr->transport_address = 0;
   /* Null out the element array */
   for (i=0; i < MAX_SUR; i++) {
      for (j=0;j<=MAX_SECTION; j++) {
         ac_device_ptr->elements[i][j] = NULL;
      }
      ac_device_ptr->phys_drv_state[i] = 0;
   }
   ac_device_ptr->storage_element_offset = 0;
   ac_device_ptr->ocl = 0;
   ac_device_ptr->pmount_lock = 0;
   ac_device_ptr->read_element_status_flag = 0;
   ac_device_ptr->transport_busy = 0;
   ac_device_ptr->rezero_lock = 0;
   ac_device_ptr->deadlock.flags = 0;
   ac_device_ptr->deadlock.recover_func = initiate_deadlock_recovery;
   ac_device_ptr->deadlock.dl_data = (caddr_t)ac_device_ptr;
} 

/*
 * This procedure creates and initializes a section structure
 */
struct section_struct*
init_section_struct()
{
   struct section_struct *ac_section_ptr;
   if ((ac_section_ptr = (struct section_struct *)kmem_alloc(
         sizeof(struct section_struct))) == NULL) {
      panic("Error with kmem_alloc in init_section_struct");
   }
   ac_section_ptr->mount_ptr = NULL;
   ac_section_ptr->pmount    = NULL;
   ac_section_ptr->punmount  = NULL;
   ac_section_ptr->open_count = 0;
   ac_section_ptr->section_flags = 0;
   ac_section_ptr->exclusive_lock = 0;

   return(ac_section_ptr);

}

/*
 * This procedure creates and initializes an ac_device structure
 */
struct ac_device *
init_ac_device()
{
   int i,j;
   struct ac_device *ac_device_ptr;
   struct move_medium_parms *move_parms_ptr;
   extern struct buf *geteblk();
   extern struct xport_request *init_xport_request();

   if ((ac_device_ptr = (struct ac_device *)kmem_alloc(
         sizeof(struct ac_device))) == NULL) {
      panic("Error with kmem_alloc in init_ac_device");
   }

   zero_ac_device(ac_device_ptr);

   /* initialize the ac_device structure */
   ac_device_ptr->forw = NULL;
   ac_device_ptr->back = NULL;
   ac_device_ptr->device = (dev_t)0;
#ifndef _WSIO
   ac_device_ptr->logical_unit = 0;
#endif
   for (i = 0; i < NUM_DRIVE; i++) {
      if ((move_parms_ptr = (struct move_medium_parms *)kmem_alloc(
            (u_int)sizeof(struct move_medium_parms))) == NULL) {
         panic("Error with kmem_alloc in init_ac_device");
      }
      ac_device_ptr->ac_drive_table[i].move_parms_ptr = move_parms_ptr;
   }

   for (i = 0; i < MAX_SUR; i++) {
      ac_device_ptr->ac_wait_time[i] = DEFAULT_WAIT_TIME;
      ac_device_ptr->ac_hog_time[i] = DEFAULT_HOG_TIME;
   }

   ac_device_ptr->ac_close_time = DEFAULT_CLOSE_TIME;
   ac_device_ptr->recover_reqp = init_xport_request();
   ac_device_ptr->jammed = 0;
   ac_device_ptr->xport_bp = NULL;
   ac_device_ptr->spinup_bp = NULL;

   return (ac_device_ptr);
} 


/*
 * This procedure creates and initializes xport_request structure.
 */
struct xport_request *
init_xport_request()

{
   struct xport_request *xport_reqp;
   if ((xport_reqp = (struct xport_request *)kmem_alloc 
              ((u_int)sizeof (struct xport_request))) == NULL) {
      panic ("Error with kmem_alloc in init_xport_request");
   } 
   xport_reqp->forw           = NULL;    
   xport_reqp->back           = NULL;    
   xport_reqp->move_request.transport   = 0;
   xport_reqp->move_request.source      = 0;
   xport_reqp->move_request.destination = 0;
   xport_reqp->move_request.invert      = 0;
   xport_reqp->sreqp          = NULL;
   xport_reqp->bp             = NULL;
   xport_reqp->drive_ptr      = NULL;
   xport_reqp->x_flags        = 0;
   xport_reqp->error          = 0;
   xport_reqp->surface        = 0;
   xport_reqp->pid            = 0;
   return(xport_reqp);
}


/*
 * Return the total of all the open counts on a surface.
 */

int
total_surface_count(ac_device_ptr,surface)
   struct ac_device *ac_device_ptr;
   int surface;

{
   int i;
   int sum = 0;
   for (i=0;i<=MAX_SECTION;i++) {
      if (ac_device_ptr->elements[surface][i])
         sum += ac_device_ptr->elements[surface][i]->open_count;

   }
   return(sum);
}


/*
 * Return the total of all the surface open counts on a given element
 */
int
total_element_count(ac_device_ptr, element)
   struct ac_device *ac_device_ptr;
   int element;
{
   int a_side,b_side;
   int sum=0;
   /* surface 1 is really element 11, side a */
   a_side = ELTOSUR(element,ac_device_ptr->storage_element_offset,0);
   b_side = ELTOSUR(element,ac_device_ptr->storage_element_offset,1);
   sum += total_surface_count(ac_device_ptr,a_side);
   sum += total_surface_count(ac_device_ptr,b_side);
   return(sum);
}

/*
 * Return the total of all the surface open counts
 */
int
total_open_cnt(ac_device_ptr)
   struct ac_device *ac_device_ptr;
{
   int i,j,sum = 0;
   for (i = 0; i < MAX_SUR; i++) 
      for (j=0;j <=MAX_SECTION; j++)
         if (ac_device_ptr->elements[i][j])
            sum += ac_device_ptr->elements[i][j]->open_count;
   return(sum);
}


/*
 * This routine returns NULL if the autochanger associated with surface_dev
 * has NEVER been opened, if it HAS been opened before it returns a pointer to
 * the ac_device structure.
 */
struct ac_device *
ac_previously_opened (surface_dev)
   dev_t surface_dev;
{
   struct ac_device *ac_device_ptr;

   /* loop thru the list of ac_devices */
   ac_device_ptr = ac_device_head;
   if (ac_device_ptr != NULL) {
      do {
#ifndef _WSIO
         /* Check for matching logical unit */
         if (ac_addr(surface_dev) == ac_device_ptr->logical_unit) {
            return (ac_device_ptr);
         }
#else
         if ((ac_card(ac_device_ptr->device) == ac_card(surface_dev)) &&
             (m_busaddr(ac_device_ptr->device) == ac_addr(surface_dev))) {
            return (ac_device_ptr);
         }
#endif
        ac_device_ptr = ac_device_ptr->forw;
      } while (ac_device_ptr != ac_device_head);
   }
   return(NULL);
}


/*
 * This routine returns NULL if the autochanger associated with surface_dev
 * is NOT opened, if it IS opened it returns a pointer to
 * the ac_device structure.
 */
struct ac_device *
ac_opened (surface_dev)
   dev_t surface_dev;
{
   struct ac_device *ac_device_ptr;

   ac_device_ptr = ac_previously_opened(surface_dev);
   if (ac_device_ptr != NULL) {
      if (ac_device_ptr->opened)
         return(ac_device_ptr);
      else
         return(NULL);
   } else {
      return(NULL);
   }
}
/*
 * This procedure rereserves the elements in the autochanger on a 
 * device powerfailure.  
 */

void
rereserve(ac_device_ptr)
   struct ac_device *ac_device_ptr;
{
   int error = 0;
   int i;
   int element;

   /* Powerfail recovery not implemented on series 300 */

#ifndef _WSIO
   /* First clear the error in the autox0 driver */
   error = autox0_ioctl(ac_device_ptr->device,AUTOX0_CLEAR_RESET);
   /* Should never return an error */
   
   error = 0;
   /* now rereserve all the drives and all the cartridges */
   do {
      error = reserve_drives(ac_device_ptr);
      for (i=0;i<MAX_SUR-1;i++) {
         element=SURTOEL(i,ac_device_ptr->storage_element_offset);
         if (total_element_count(ac_device_ptr, element) != 0) 
            error = reserve_element(ac_device_ptr, element);
      } 
   } while (error == ECONNRESET);
#endif
}

/*
 *  This routine sends a reset_ac to the xport daemon
 */

int
ac_send_reset(ac_device_ptr)
    struct ac_device *ac_device_ptr;

{
   struct xport_request *xport_reqp;
   int error;
   int pri;

   xport_reqp = init_xport_request();
   xport_reqp->x_flags = X_RESET_AC;
   ac_device_ptr->transport_busy = 1;
   ADD_TO_LIST(ac_device_ptr->xport_queue_head,xport_reqp);
   pri = spl6();
   wakeup(&(ac_device_ptr->xport_queue_head));
   sleep(xport_reqp,PRIBIO);
   (void) splx(pri);
   error = xport_reqp->error;
   FREE_ENTRY(xport_reqp,xport_request);
   return(error);
}

/*
 * This procedure does a SCSI release on an element in the autochanger.
 */
int
release_element(ac_device_ptr, element)
   struct ac_device *ac_device_ptr;
   int element;
{
   int error=0;
   struct reserve_parms *release;
   extern struct buf *geteblk();

   if ((release = (struct reserve_parms *)kmem_alloc(
         (u_int)sizeof(struct reserve_parms))) == NULL) {
      panic("Error with kmem_alloc in release_element");
   }

   /* load the release structure with element to release */
#ifndef _WSIO
   release->element = element;
#else /* s300 and s700 */
   release->element = 1;   /* signify that this is an ELEMENT release */
#endif

   /* use element number as reservation id */
   release->resv_id = element;

   /* do release ioctl */
#ifdef KERNEL_DEBUG_ONLY
   if (force_error.release_element) error = EIO;
   else
#endif KERNEL_DEBUG_ONLY
   error = autox0_ioctl(ac_device_ptr->device,AUTOX0_RELEASE,release);
   while (error == ECONNRESET) {
      rereserve(ac_device_ptr); 
      error = autox0_ioctl(ac_device_ptr->device,AUTOX0_RELEASE,release);
   }
   kmem_free((caddr_t)release,(u_int)sizeof(struct reserve_parms));
   return(error);
}


/*
 * This procedure does a SCSI reserve on an element in the autochanger.
 */
int
reserve_element(ac_device_ptr, element)
   struct ac_device *ac_device_ptr;
   int element;
{
   int error=0;
   struct reserve_parms *reserve;

   if ((reserve = (struct reserve_parms *)kmem_alloc(
         (u_int)sizeof(struct reserve_parms))) == NULL) {
      panic("Error with kmem_alloc in reserve_element");
   }
   /* load the reserve structure with element to reserve */
#ifndef _WSIO
   reserve->element = element;
#else /* s300 and s700 */
   reserve->element = 1;  /* signify that this is an ELEMENT reservation */
#endif

   /* use element number as reservation id */
   reserve->resv_id = element;

      /* send one reservation */
   reserve->ell = RESERVE_ELEMENT_LIST_LENGTH;

#ifdef KERNEL_DEBUG_ONLY
   if (force_error.reserve_element) error = EIO;
   else
#endif KERNEL_DEBUG_ONLY
   error = autox0_ioctl(ac_device_ptr->device,AUTOX0_RESERVE,reserve);
   while (error == ECONNRESET) {
      rereserve(ac_device_ptr);
      error = autox0_ioctl(ac_device_ptr->device,AUTOX0_RESERVE,reserve);
   }
   kmem_free((caddr_t)reserve,(u_int)sizeof(struct reserve_parms));
   return(error);
}

/*
 * This procedure does a SCSI release on all drives in the autochanger.
 */
int
release_drives(ac_device_ptr)
   struct ac_device *ac_device_ptr;
{
   int i,error=0;
   struct reserve_parms *release;

   if ((release = (struct reserve_parms *)kmem_alloc(
         (u_int)sizeof(struct reserve_parms))) == NULL) {
      panic("Error with kmem_alloc in release_drives");
   }
   for (i=0; i<ac_device_ptr->number_drives; i++) {
      /* signify that this is an ELEMENT release */
#ifndef _WSIO
      release->element = ac_device_ptr->ac_drive_table[i].element_address;
#else /* s300 and s700 */
      release->element = 1;   
#endif

      /* reservation id for element to be released is element_addr */
      release->resv_id = ac_device_ptr->ac_drive_table[i].element_address;

      /* do release ioctl */
#ifdef KERNEL_DEBUG_ONLY
      if (force_error.release_drives) error = EIO;
      else
#endif KERNEL_DEBUG_ONLY
      error = autox0_ioctl(ac_device_ptr->device,AUTOX0_RELEASE,release);
      while (error == ECONNRESET) {
         rereserve(ac_device_ptr);
         error = autox0_ioctl(ac_device_ptr->device,AUTOX0_RELEASE,release);
      } 
      if (error != 0)
         break;
   }
   kmem_free((caddr_t)release,(u_int)sizeof(struct reserve_parms));

   return(error);

}

/*
 * This procedure does a SCSI reserve on all drives in the autochanger.
 */
int
reserve_drives(ac_device_ptr)
   struct ac_device *ac_device_ptr;
{
   int i,error=0;
   struct reserve_parms *reserve;

   if ((reserve = (struct reserve_parms *)kmem_alloc(
         (u_int)sizeof(struct reserve_parms))) == NULL) {
      panic("Error with kmem_alloc in reserve_drives");
   }

   for (i=0; i<ac_device_ptr->number_drives; i++) {
      /* signify that this is an ELEMENT reservation */
#ifndef _WSIO
      reserve->element = ac_device_ptr->ac_drive_table[i].element_address;
#else /* s300 and s700 */
      reserve->element = 1;
#endif

      /* reservation id is element_addr */
      reserve->resv_id = ac_device_ptr->ac_drive_table[i].element_address;

      /* send one reservation */
      reserve->ell = RESERVE_ELEMENT_LIST_LENGTH;

#ifdef KERNEL_DEBUG_ONLY
      if (force_error.reserve_drives) error = EIO;
      else
#endif KERNEL_DEBUG_ONLY
      error = autox0_ioctl(ac_device_ptr->device,AUTOX0_RESERVE,reserve);
      while (error == ECONNRESET) {
         rereserve(ac_device_ptr);
         error = autox0_ioctl(ac_device_ptr->device,AUTOX0_RESERVE,reserve);
      }
      if (error != 0)
         break;

   }
   kmem_free((caddr_t)reserve,(u_int)sizeof(struct reserve_parms));
   return(error);
}



/*
 * routine to find the proper status page in a full status dump...
 * skips over data blocks with incorrect page headers
 *
 */

unsigned char *
find_status_page(status_info,page_code)

unsigned char   *status_info;
int     page_code;

{
   unsigned char *current_info = status_info;
   int  current_length;
   int  status_left =      (status_info[5] << 16) |
            (status_info[6] << 8)  |
            status_info[7];

   for (   current_info +=8; status_left > 0;
      current_info += current_length, status_left -= current_length) {

      current_length = (      (current_info[5] << 16) |
               (current_info[6] << 8)  |
               current_info[7]) +8;

      if (current_info[0] == page_code)
         return(current_info);
   }


   msg_printf1("AC: autochanger did not return status page %d\n",page_code);
   panic("Autochanger: device did not return status page");

   return(0);
}

/*
 * This procedure checks the consistency of the autochanger.
 * Is the transport full?
 * Is any drive full with out a vaild source?
 * Is any SCSI address a duplicate?
 */
int
ac_consistency_check(ac_device_ptr, status_info)
   struct ac_device *ac_device_ptr;
   unsigned char *status_info;
{
   int el_desc_len, j;
   unsigned char   *transport_info;
   unsigned char   *drive_info;
   unsigned char   *drive_entry;
   struct transport_element_desc {
      unsigned char   elem_address_msb,
            elem_address_lsb,
            elem_status,
            reservd;
      } *transport_desc;
   int     elements,current_element,i;

   /* check to see if the transport is full */
   transport_info = find_status_page(status_info,TRANSPORT_PC);

   elements = (    (transport_info[5] << 16) |
         (transport_info[6] << 8)  |
         transport_info[7]
         ) / sizeof (struct transport_element_desc);

   transport_desc = (struct transport_element_desc *)&(transport_info[8]);
   for (i = 0; i < elements; i++,transport_desc++)
      {

      if(transport_desc->elem_status & FULL_MSK) {
         printf("autochanger at dev 0x%x is in an inconsistant state:\n",
            ac_device_ptr->device);
#ifndef _WSIO
         printf("autochanger is logical unit number %d \n",ac_device_ptr->logical_unit);
#endif
         printf("   transport is full\n");
            return(ENXIO);
      }
   }


   /* check to see if any drive is full with an invalid source */

   drive_info = find_status_page(status_info,DATA_TRANSFER_PC);

   /* get element_descriptor length for use as offset */
   el_desc_len =
      MAKE_WORD(drive_info[2],drive_info[3]);

   /* number of drives is the number of elements */
   /* which is the number of data bytes / length of desc -paulc */
   ac_device_ptr->number_drives = (
         (drive_info[5] << 16) |
         (drive_info[6] << 8)  |
         drive_info[7]
         ) / el_desc_len;

   /* loop thru the element descriptors to get the other info */
   for (j = 0, drive_entry = &(drive_info[8]);
      j < ac_device_ptr->number_drives;
      drive_entry+=el_desc_len, j++) {

      /* make sure that the scsi address is good */
      if (!(drive_entry[6] & ID_VAL_MSK)) {
         /* if any address is not set error */
         return (ENXIO);
      }

      /* make sure address is on the same bus */
      if (drive_entry[6] & NOT_THIS_BUS_MSK) {
         return (ENXIO);
      }

      /* get scsi address of element */
      ac_device_ptr->ac_drive_table[j].scsi_address =
         drive_entry[7];

      /* loop thru the drives to see if a duplicate SCSI ID exists */
      for(i = 0; i < j; i++) {
         if(ac_device_ptr->ac_drive_table[i].scsi_address == 
            ac_device_ptr->ac_drive_table[j].scsi_address) {
            printf("autochanger at dev %x is in an inconsistant state:\n",
               ac_device_ptr->device);
#ifndef _WSIO
            printf("autochanger is logical unit number %d \n",ac_device_ptr->logical_unit);
#endif
            printf("   TWO drives at the SAME SCSI ID (%d)\n",
               ac_device_ptr->ac_drive_table[j].scsi_address);
            return (ENXIO);
         }
      }

#ifndef _WSIO
      /* Can't check the autochanger ID because all I have is the LU number */
#else /*  s300 and s700 */
      /* check to see if the autochanger is at the same ID as a drive */
      if(ac_device_ptr->ac_drive_table[j].scsi_address == m_busaddr(ac_device_ptr->device)) {
         printf("autochanger at dev %x is in an inconsistant state:\n",
            ac_device_ptr->device);
         printf("   autochanger and drive at the SAME SCSI ID (%d)\n",         
            ac_device_ptr->ac_drive_table[j].scsi_address);
         return (ENXIO);
      }
#endif

      /* get source if valid */
      if (drive_entry[2] & FULL_MSK) {
         if (!(drive_entry[9] & SOURCE_VAL_MSK)) {
            printf("autochanger at SCSI dev %x is in an inconsistant state:\n",
               ac_device_ptr->device);
#ifndef _WSIO
         printf("autochanger is logical unit number %d \n",ac_device_ptr->logical_unit);
#endif
            printf("   media in drive at SCSI ID (%d) has no source\n",
               ac_device_ptr->ac_drive_table[j].scsi_address);
            return (ENXIO);
         } else {  /* check to make sure the source is not: 
              1).  mail slot 
                      2).  a slot not valid in the autochanger 

           Assumes the data structures in the ac_device and drives 
                   have been set. */

            if (SURTOEL(ac_device_ptr->ac_drive_table[j].surface,ac_device_ptr->storage_element_offset) == 
                ac_device_ptr->mailslot_address ) {
               printf("autochanger at SCSI dev %x is in an inconsistant state:\n",
                  ac_device_ptr->device);
#ifndef _WSIO
         printf("autochanger is logical unit number %d \n",ac_device_ptr->logical_unit);
#endif
               printf("   media in drive at SCSI ID (%d) has the mailslot as its source\n",
                  ac_device_ptr->ac_drive_table[j].scsi_address);
               printf("Please eject the cartridge from the drive and load it into a slot\n");
               return (ENXIO);
               }

            if (ac_device_ptr->ac_drive_table[j].surface < 0 ) {
               printf("autochanger at SCSI dev %x is in an inconsistant state:\n",
                  ac_device_ptr->device);
#ifndef _WSIO
         printf("autochanger is logical unit number %d \n",ac_device_ptr->logical_unit);
#endif
               printf("   media in drive at SCSI ID (%d) has an invalid source\n",
                  ac_device_ptr->ac_drive_table[j].scsi_address);
               printf("Please eject the cartridge from the drive and load it into a slot\n");
               return (ENXIO);
               }

         }
      }
   }

   return(0);
}

void
close_file_desc()
{
   int i;

/* copied from the exit code */

    for (i = 0; i <= u.u_highestfd; i++) {
        struct file *f;

        f = getf(i);
        if (f != NULL) {
#ifdef __hp9000s300
            dil_exit(f);
#endif
#ifndef NFS3_2
            uffree(i);
#endif
            /*
             * Guard againt u.u_error being set by psig()
             * u.u_error should be 0 if exit is called from
             * syscall().  In the case of exit() being called
             * by psig - u.u_error could be set to EINTR.  We
             * need to clear this so that routines called by
             * exit() would't return on non-zero u.u_error.
             * Example: vno_lockrelease()
             */
            u.u_error = 0;
            closef(f);
#ifdef NFS3_2
            /*
             * Save flags until after closef() because flags
             * are used in vno_lockrelease, called from closef().
             */
            uffree(i);
#endif /* NFS3_2 */
            KPREEMPTPOINT();
        }
        else {
            /*
             * If f is NULL, then getf has set u.u_error to EBADF.
             * Since this just means that this process doesn't
             * have a file descriptor "i" opened, we should
             * ignore this error.
             */
            u.u_error = 0;
        }
    }
    release_cdir();
    if (u.u_rdir) {
        update_duxref(u.u_rdir, -1, 0);
        VN_RELE(u.u_rdir);
    }
}


/*
 * Fork the xport and spinup daemons for this autochanger.
 */

int
ac_fork_daemons(ac_device_ptr)
struct ac_device *ac_device_ptr;

{
   register proc_t *p;
   int xnewpid;
   int snewpid;
   int ret;
   proc_t *xcp,*scp;
   extern void xport_daemon();
   extern void spinup_daemon();
   extern void pstat_cmd();


   /* make sure we can get two new procs */
   xnewpid = getnewpid();
   if (xnewpid < 0)  /* no procs */
      return(xnewpid);
   snewpid = getnewpid();
   if (snewpid < 0)  /* no procs */
      return(snewpid);

   /* Start up the daemons */

   xcp = allocproc(S_DONTCARE);
   if (xcp == NULL) 
      return(FORKRTN_ERROR);

   scp = allocproc(S_DONTCARE);
   if (scp == NULL) {
      freeproc(xcp);
      return(FORKRTN_ERROR);
   }

   switch (newproc(FORK_DAEMON,xcp)) {
   case FORKRTN_ERROR:
      freeproc(xcp);
      freeproc(scp);
      return(FORKRTN_ERROR);
      break;
   case FORKRTN_PARENT:
      /* Parent process returns here */
      break;
   case FORKRTN_CHILD:
      p = u.u_procp;
#ifndef _WSIO
      MP_PSEMA(&up_io_sema);
#endif
      /* get the I/O semaphore explicitly, since the child does not
       * inherit the semaphores held by the parent.
       * Note:  this should be removed when autoch goes MP
       */
      proc_hash(p,0,xnewpid,PGID_NOT_SET,SID_NOT_SET);
      pstat_cmd(u.u_procp,"xportd",1,"xportd");
      u.u_syscall = KI_XPORTD;
      close_file_desc();
      xport_daemon(ac_device_ptr);
      break;
   }

#ifdef KERNEL_DEBUG_ONLY
   if (force_error.kill_xportd) ret = FORKRTN_ERROR;
   else
#endif KERNEL_DEBUG_ONLY
   ret = newproc(FORK_DAEMON,scp);
   switch (ret) {
   case FORKRTN_ERROR:
      /* if xport daemon is started and spinup daemon fails, kill xportd */
      ac_kill_xportd(ac_device_ptr);
      freeproc(scp);
      return(FORKRTN_ERROR);
      break;
   case FORKRTN_PARENT:
      /* Parent process returns here */
      return(0);
      break;
   case FORKRTN_CHILD:
      p = u.u_procp;
#ifndef _WSIO
      MP_PSEMA(&up_io_sema);
#endif
      /* get the I/O semaphore explicitly, since the child does not
       * inherit the semaphores held by the parent.
       * Note:  this should be removed when autoch goes MP
       */
      proc_hash(p,0,snewpid,PGID_NOT_SET,SID_NOT_SET);
      pstat_cmd(u.u_procp,"spinupd",1,"spinupd");
      u.u_syscall = KI_SPINUPD;
      close_file_desc();
      spinup_daemon(ac_device_ptr);
      break;
   }
}

/*
 * Puts a request in the xport queue to kill the xport
 * daemon.  This is done in the case where the xport daemon
 * starts up but the spinup daemon fails to start.  Then the
 * xport daemon must be killed.
 */

int
ac_kill_xportd(ac_device_ptr)
   struct ac_device *ac_device_ptr;

{
   struct xport_request *xport_reqp;
   int error;
   int pri;

   xport_reqp = init_xport_request();   /* no move parameters necessary */
   xport_reqp->pid = 0;
   xport_reqp->x_flags = X_KILL_XPORTD;
   ac_device_ptr->transport_busy = 1;
   ADD_TO_LIST(ac_device_ptr->xport_queue_head,xport_reqp);
   pri = spl6();
   wakeup(&(ac_device_ptr->xport_queue_head));
   sleep(xport_reqp,PRIBIO);
   splx(pri);
   error = xport_reqp->error;
   FREE_ENTRY(xport_reqp,xport_request);
   return(error);
}

/*
 * Does a release of all cartridges in the autochanger.
 * Necessary if the autochanger is set to remember the
 * reservations made by the host across power failures.
 * If power is lost, the user would have mount then 
 * unmount the cartridge before it could be ejected
 * via the front panel.
 */

void
release_all_cartridges(ac_device_ptr)
   struct ac_device *ac_device_ptr;

{
   int el_desc_offset;
   int element, el_desc_len, error = 0, page_code, i;
   int number_of_surfaces = 0;
   struct buf *el_bp;
   unsigned char *status_info;
   unsigned char *storage_info;
   extern void ac_read_element_status();

/*
 * get element status atomically, to avoid syncronization problems
 * which can occur when the autochanger is in motion
 * -paulc 10/26/1989
 */
   el_bp      = geteblk(EL_STATUS_SZ);
   el_bp->b_dev    = ac_device_ptr->device;
   page_code       = ALL_PC;
   ac_read_element_status(ac_device_ptr,el_bp,page_code);
   if (error = geterror(el_bp)) {
      brelse(el_bp);
      return;
   }

   status_info = (unsigned char *)el_bp->b_un.b_addr;

   storage_info = find_status_page(status_info,STORAGE_PC);

   /* get element_descriptor length for use as offset */
   el_desc_len = MAKE_WORD(storage_info[2],storage_info[3]);

   /* calculate number of surfaces */
   number_of_surfaces = (
         (storage_info[5] << 16) |
         (storage_info[6] << 8)  |
         storage_info[7] ) / el_desc_len;
   if (AC_DEBUG)
      msg_printf1("Releasing %d elements\n",number_of_surfaces);
   brelse(el_bp);
   

   for(i=0;i<number_of_surfaces;i++) {
      element = i + ac_device_ptr->storage_element_offset;
      /* ignore the error */
      error = release_element(ac_device_ptr,element);
      if (AC_DEBUG)
         msg_printf1("Released element %d \n",element);
   }
}


/*
 * Do initial open of the autochanger
 */
int
open_autochanger(surface_dev,ac_ptr_ptr)
   dev_t surface_dev;
   struct ac_device **ac_ptr_ptr;
{
   dev_t ac_dev;
   struct ac_device *ac_device_ptr;
   struct ac_drive *drive_ptr;
   struct timeval atv;
   int i,error_switch = 0;
   int error = 0;
   int escapecode = 0;
#ifndef _WSIO
   io_tree_entry *nodep;
   extern int map_lu_to_mi();
#endif
   extern void ac_read_element_status();
   void autoch_clear_phys_drive();

   struct buf      *el_bp = NULL;  /* element status bp...temp */
   int        page_code;      /* element status page code */

   try

   if ((*ac_ptr_ptr = ac_previously_opened(surface_dev)) == NULL) {
      /* allocate the new ac_device */
      *ac_ptr_ptr = init_ac_device();

      /* put the ac_device structure on the linked list */
      ADD_TO_LIST(ac_device_head,(*ac_ptr_ptr));
   } else {
      zero_ac_device(*ac_ptr_ptr);
   }
   ac_device_ptr = *ac_ptr_ptr;

   /* mark the ac open */
   ac_device_ptr->opened = 1;


   /* make a dev for the mechanical changer */
#ifndef _WSIO
   ac_device_ptr->logical_unit = ac_addr(surface_dev);

   ac_dev = makedev(AUTOX0_MAJ_CHAR,makeminor(0,ac_addr(surface_dev),0,0));
   error = map_lu_to_mi(&ac_dev,IFCHR,&nodep);
   if (error) {
      msg_printf0("map_lu_to_mi failed");
      error = EIO;
   }
#else
   ac_dev = makedev(AUTOX0_MAJ_CHAR, 
                    ac_minor(ac_card(surface_dev), ac_addr(surface_dev), 0, 0));
#endif

   /* put in the proper device */
   ac_device_ptr->device = ac_dev;

   /* open the mechanical changer */
#ifdef KERNEL_DEBUG_ONLY
   if (force_error.autoch_scsi_open) error = EIO;
   else
#endif KERNEL_DEBUG_ONLY

   if (error == 0) {
#ifndef _WSIO
      error = autox0_open (ac_dev);
#else /* s300 and s700 */
      error = autox0_open (ac_dev, 0);   /* flag = 0 not used */
#endif
   }
   if (error) {
      error_switch = 1;
      escape(error);
   }

/*
 * get element status atomically, to avoid syncronization problems
 * which can occur when the autochanger is in motion
 * -paulc 10/26/1989
 */
   el_bp      = geteblk(EL_STATUS_SZ);
   el_bp->b_dev    = ac_device_ptr->device;
   page_code       = ALL_PC;
   ac_read_element_status(ac_device_ptr,el_bp,page_code);
   if (error = geterror(el_bp)) {
      error_switch = 2;
      escape (error);
   }

   get_storage_element_offset(ac_device_ptr,(unsigned char *)el_bp->b_un.b_addr);
   get_transport_address(ac_device_ptr,(unsigned char *)el_bp->b_un.b_addr);

   if (error = get_drive_info(ac_device_ptr,(unsigned char *)el_bp->b_un.b_addr)) {
      error_switch = 4;
      escape (error);
   }

   get_mailslot_address(ac_device_ptr,(unsigned char *)el_bp->b_un.b_addr);

   if(error = ac_consistency_check(ac_device_ptr,(unsigned char *)el_bp->b_un.b_addr)) {
      error_switch = 3;
      escape(error);
   }

   /* set hog_time_start of inserted surfaces */
   GET_TIME(atv);
   for (i = 0; i < ac_device_ptr->number_drives; i++) {
      if (ac_device_ptr->ac_drive_table[i].surface != EMPTY) {
         ac_device_ptr->ac_drive_table[i].hog_time_start = atv.tv_sec;
         drive_ptr = &(ac_device_ptr->ac_drive_table[i]);
         drive_ptr->surface_inproc = ac_device_ptr->ac_drive_table[i].surface;
         error = do_spinup(drive_ptr,
          ac_device_ptr->ac_drive_table[i].surface);
         /* spinup the surfaces that are in the drives to get the
         data structures set correctly */
         if (error)  {
            printf("Drive %d failed to spinup in dev = 0x%x \n",i,ac_dev);
#ifndef _WSIO
         printf("autochanger is logical unit number %d \n",ac_device_ptr->logical_unit);
#endif
            error_switch = 5;
            escape (error);
         }
         else {
#ifndef _WSIO
            autoch_clear_phys_drive(makedev(drive_maj_char, makeminor(0,drive_ptr->logical_unit,0,2)),drive_ptr,INIT);
#else
            autoch_clear_phys_drive(makedev(drive_maj_blk, ac_minor(ac_card(drive_ptr->ac_device_ptr->device),
                                            drive_ptr->scsi_address,0,0)),drive_ptr,INIT);
#endif
            drive_ptr->drive_state = NOT_BUSY;
            /* now close the drive */
         }
      }
   }

   release_all_cartridges(ac_device_ptr);

   if(error = reserve_drives(ac_device_ptr)) {
      error_switch = 6;
      escape(error);
   }
   
#ifdef KERNEL_DEBUG_ONLY
   if (force_error.no_daemons) error = EIO;
   else
#endif KERNEL_DEBUG_ONLY
   error = ac_fork_daemons(ac_device_ptr);
   if (error) {
      error_switch = 7;
      escape(error);
   }

   brelse(el_bp);

   return(0);
   
   recover: {
      if (AC_DEBUG&PANICS) panic("error in open_autochanger");
      switch (error_switch) {
         /*it is intended that each item fall thru to the next*/
         /*this will undo everything in reverse order*/
     case 7:
         case 6: release_drives(ac_device_ptr);
         case 5: /* ERROR: CLOSE ANY OF THE DRIVES THAT WERE SUCCESSFULLY OPENED */
            for (i=0; i<ac_device_ptr->number_drives; i++) {
               if ((ac_device_ptr->ac_drive_table[i].opened != 0) && 
                   (ac_device_ptr->ac_drive_table[i].surface != EMPTY)) {
                  /* close the drives that were opened */
                  drive_ptr = &(ac_device_ptr->ac_drive_table[i]);
#ifndef _WSIO
                  (*cdevsw[drive_maj_char].d_close)(makedev(drive_maj_char,
                     makeminor(0,drive_ptr->logical_unit,0,2)));
#else
                  (*cdevsw[drive_maj_char].d_close)(makedev(drive_maj_char,
                     ac_minor(ac_card(ac_device_ptr->device),
                     drive_ptr->scsi_address,0,0)),0);
#endif
                  if (AC_DEBUG & OPERATION)
                     msg_printf0("AUTOCH_CLOSE: Final close of drives\n");
                  drive_ptr->opened = 0;
               }
            }
         case 4:
         case 3:
         case 2: /* close the autochanger */
#ifndef _WSIO
            (*cdevsw[major(ac_dev)].d_close)(ac_dev);
#else /* s300 and s700 */
            (*cdevsw[major(ac_dev)].d_close)(ac_dev, 0);
#endif
            brelse(el_bp);
         case 1: ac_device_ptr->opened = 0;
            break;
         default:
            panic("Bad error recovery in open_autochanger");
      }
      return(escapecode);
   }
}
#ifndef _WSIO
extern int nodev();
#endif

int
autoch_open (dev,flag)
   dev_t dev;
   int flag;
{
   int error_switch, surface, error=0, element;
   int i,section;
   int escapecode = 0;
   int ac_just_opened = 0;
   dev_t ac_dev;
   struct timeval atv;
   struct ac_device *ac_device_ptr;
   struct ac_drive *drive_ptr;
   extern struct ac_device *ac_opened ();
   extern dev_t preprocess_dev();
   extern void autox0_close();
   extern void pmount_section();
   int pri; /* used to make sure the open count is not
           corrupted during exclusive opens */

   if (ON_ICS)
   {
       panic("autoch_open: called ON_ICS");
   }

   /* determine which surface */
   surface = ac_surface (dev);

   /* make a dev for the mechanical changer */
#ifndef _WSIO
   /* make sure autox0 and disc3 genned into kernel */
   if (cdevsw[SCSI_MAJ_CHAR].d_open == nodev) {
    printf("autoch_open: disc3 not configured into kernel\n");
    error = ENODEV;
   }
   if (cdevsw[AUTOX0_MAJ_CHAR].d_open == nodev) {
    printf("autoch_open: autox0 not configured into kernel\n");
    error = ENODEV;
   }
   if (error == ENODEV) {
         if (ac_open_init & INITIALIZING_WANTED) {
            wakeup((caddr_t)&ac_open_init);
         }
         ac_open_init = 0;
         return(ENODEV);
   }

   ac_dev = makedev(AUTOX0_MAJ_CHAR,makeminor(0,ac_addr(dev),0,0));
#else
   ac_dev = makedev(AUTOX0_MAJ_CHAR,ac_minor(ac_card(dev),ac_addr(dev),0,0));
#endif

   if ((surface == 0) && (major(ac_dev) == AC_MAJ_BLK)) {
         /* can't open block autochanger device */
         return(ENXIO);
   }

   /* check open lock */
   while ((ac_open_init & INITIALIZING) || 
          ((ac_device_ptr = ac_previously_opened(dev)) &&
           (ac_device_ptr->ocl & (IN_CLOSE | IN_OPEN)))) {

      /* you are locked out, please wait */
      ac_open_init |= INITIALIZING_WANTED;
      error = sleep(&ac_open_init,PLOCK|PCATCH);
      if (error) {
         /* user has interrupted my sleep */
         if (ac_open_init & INITIALIZING_WANTED) {
            wakeup((caddr_t)&ac_open_init);
         }
         ac_open_init = 0;
         return(EINTR);
      }
   }

try
   /* has ac been opened yet? */
   if ((ac_device_ptr = ac_opened(dev)) == NULL) {
      /* lock out other open calls */
      ac_open_init |= INITIALIZING;

      error = open_autochanger(dev,&ac_device_ptr);
      ac_just_opened = 1;

      /* wakeup anybody waiting to open autochanger*/
      if (ac_open_init & INITIALIZING_WANTED) {
         wakeup((caddr_t)&ac_open_init);
      }
      ac_open_init = 0;

      if (error) {
         error_switch = 1;
         escape(error);
      }

   }

   /* synchronize with other open/close routines */

   if (AC_DEBUG & OPEN_CLOSE)
      msg_printf1("%d   autoch_open: opening. surface %x\n", u.u_procp->p_pid, surface);
   while ( ac_device_ptr->ocl & (IN_CLOSE | IN_OPEN) ) {

      ac_device_ptr->ocl |= WANT_OPEN;
#ifdef KERNEL_DEBUG_ONLY
      if (force_error.ac_open_sleep) {
         error = -1;
         psignal(u.u_procp,SIGINT);
      } else     /* Bruce...this path is funny if force_error is set */
#endif KERNEL_DEBUG_ONLY
      if (AC_DEBUG & OPEN_CLOSE)
         msg_printf1("%d   autoch_open: sleeping. surface %x\n", u.u_procp->p_pid, surface);
      error = sleep(&(ac_device_ptr->ocl),PLOCK | PCATCH);
      if (error) {
         /* user has interrupted my sleep */
         error_switch = 2;
         escape(EINTR);
      }
   }

   ac_device_ptr->ocl |= IN_OPEN;
   if (AC_DEBUG & OPEN_CLOSE)
      msg_printf1("%d   autoch_open: opened. surface %x\n", u.u_procp->p_pid, surface);

   if (ac_device_ptr->jammed) {

      /* autochanger previously jammed, try to recover */
      error = ac_send_reset(ac_device_ptr);

      if (error) {
         error_switch = 3;
         escape(EIO);
      }
   }

   /* get element number */
   element = SURTOEL(surface,ac_device_ptr->storage_element_offset);

   section = ac_section(dev);
   if (surface == 0)  /* doing an ioctl open */
      section = 0;
   if ((section == 10) || (section == 11)) {
      error_switch = 4;
      escape (EINVAL);
   }
   /* increment the open count for this surface */
   if (ac_device_ptr->elements[surface][section] == NULL) {

      pri = spl6();  /* critical section for exclusive open */

      /* allocate a section structure for it */
      ac_device_ptr->elements[surface][section] = init_section_struct();
      ac_device_ptr->elements[surface][section]->open_count += 1;

#ifndef _WSIO
      if (major(dev) == AC_MAJ_BLK) {
         ac_device_ptr->elements[surface][section]->section_flags |= AC_BLK_OPEN;
      }
      else {
         ac_device_ptr->elements[surface][section]->section_flags |= AC_CHAR_OPEN;
      }
#endif /* defined(__hp9000s800) && !defined(__hp9000s700) */

      splx(pri); /* end critical section for exclusive open */

      } /* if ac_device_ptr->elements[surface][section] == NULL */
   else {
      pri = spl6();

#ifndef _WSIO

      /* The 800 only sends a driver_close on the last close of the device. */
      /* Must adjust the open count so it will match the close calls. */
      if (surface == 0)   /* this is for the autochanger */
         /* Assume that the daemons are running when we get to here */
         ac_device_ptr->elements[surface][section]->open_count = 3;
      else 

      {
         /* if the section is for the entire surface and it has been allocated */
         if ((section == ENTIRE_SURFACE)  &&  (ac_device_ptr->elements[surface][section] != NULL)) {
            if (ac_device_ptr->elements[surface][section]->exclusive_lock != 0) {
               error_switch = 5;
               splx(pri);               /* set the priority back due to error */
               escape(EACCES);
        } /* if already locked */
         } /* if valid section and allocated */

         ac_device_ptr->elements[surface][section]->open_count += 1;
         if (major(dev) == AC_MAJ_BLK) {
            ac_device_ptr->elements[surface][section]->section_flags |= AC_BLK_OPEN;
         }
         else {
            ac_device_ptr->elements[surface][section]->section_flags |= AC_CHAR_OPEN;
         }

      } /* else surface != 0 */

#else /* s300 and s700 */

      /* if the section is for the entire surface and it has been allocated */
      if ((section == ENTIRE_SURFACE)  &&  (ac_device_ptr->elements[surface][section] != NULL)) {
         if (ac_device_ptr->elements[surface][section]->exclusive_lock != 0) {
            error_switch = 5;
            splx(pri);               /* set the priority back due to error */
            escape(EACCES);
         } /* if already locked */
      } /* if valid section and allocated */
      ac_device_ptr->elements[surface][section]->open_count += 1;

#endif
      splx(pri);
   }
   if (ac_just_opened) {
      if (ac_device_ptr->elements[0][0] == NULL) {
         /* The first open might not have been for the ioctl device */
         /* allocate a section structure for it */
         ac_device_ptr->elements[0][0] = init_section_struct();
         } 
      /* adjust open count for the daemons */
      ac_device_ptr->elements[0][0]->open_count = 2;  
      if (surface == 0) {
         ac_device_ptr->elements[0][0]->open_count += 1;  
         }
      }
   if (surface != 0) { /* mark the drives in use so the ioctls won't try to use them */
      for (i=0; i<ac_device_ptr->number_drives; i++) {
         ac_device_ptr->ac_drive_table[i].inuse = 1;
         if ((ac_device_ptr->ac_drive_table[i].opened == 0) && 
             (ac_device_ptr->ac_drive_table[i].surface != EMPTY)) {
            /* Need to open the drives */
            /* set hog_time_start of inserted surfaces */
            GET_TIME(atv);
            ac_device_ptr->ac_drive_table[i].hog_time_start = atv.tv_sec;
            drive_ptr = &(ac_device_ptr->ac_drive_table[i]);
            drive_ptr->surface_inproc = ac_device_ptr->ac_drive_table[i].surface;
            error = do_spinup(drive_ptr,
            ac_device_ptr->ac_drive_table[i].surface);
            /* spinup the surfaces that are in the drives to get the
            data structures set correctly */
            if (error)  {
               printf("Drive %d failed to spinup in dev = 0x%x \n",i,ac_dev);
#ifndef _WSIO
         printf("autochanger is logical unit number %d \n",ac_device_ptr->logical_unit);
#endif
               error_switch = 6;
               escape (error);
            }
            else {
               drive_ptr->opened = 1;
               drive_ptr->drive_state = NOT_BUSY;
               /* now close the drive */
            }
         }
      }
   }


   /* check rezero lock */
   if (ac_device_ptr->rezero_lock & REZERO_BUSY) {
      /* you are locked out, please wait */
      ac_device_ptr->rezero_lock |= REZERO_WANTED;
      error = sleep(&ac_device_ptr->rezero_lock,PLOCK|PCATCH);
      if (error) {
         /* user interrupted the sleep */
         error_switch = 7;
         escape(EINTR);
      }
   }

   if (surface != 0) { /* want to open a piece of media not autochanger */
      /* check to see if surface is in the autochanger */
      if(error = surface_in_ac (ac_device_ptr, surface)) {
         error_switch = 8;
         escape(error);
      }
      /* first time that any surface on ELEMENT has been opened?? */
      if (total_element_count(ac_device_ptr, element) == 1) {
         /* if so, reserve it so users can't get it */
         if (error = reserve_element(ac_device_ptr, element)) {
            error_switch = 9;
            escape(error);
         }
      }
      /* First open of a non-spinup section with cart inserted needs opening */
      if ((ac_device_ptr->elements[surface][section]->open_count == 1) &&
          (current_sur_inserted(ac_device_ptr,surface,&drive_ptr))) {
            pmount_section(drive_ptr,section,NULL);
      }
   }

   /* wakeup anybody waiting to open/close */
   if (ac_device_ptr->ocl & (WANT_OPEN | WANT_CLOSE)) {
      if (AC_DEBUG & OPEN_CLOSE)
         msg_printf1("%d   autoch_open: waking up. surface %x\n", u.u_procp->p_pid, surface);
      wakeup((caddr_t)&(ac_device_ptr->ocl));
   }
   if (ac_open_init & INITIALIZING_WANTED) {
      wakeup((caddr_t)&ac_open_init);
   }

   ac_device_ptr->ocl = 0;

   return(0);      /* no error */

recover: {
      if (AC_DEBUG&PANICS) panic("error in autoch_open");
      switch (error_switch) {
         /*it is intended that each item fall thru to the next*/
         /*this will undo everything in reverse order*/
         case 9: /* nothing new */
         case 8: /* nothing new */
         case 7: /* nothing new */
         case 6: (ac_device_ptr->elements[surface][section]->open_count)--;
            if (ac_device_ptr->elements[surface][section]->open_count == 0) {
               FREE_ENTRY(ac_device_ptr->elements[surface][section],section_struct);   
               ac_device_ptr->elements[surface][section] = NULL;
            }
            if (total_open_cnt(ac_device_ptr) == 2) { /*only the daemons running */
               /* mark the drives in use so the ioctls can use them */
               for (i=0; i<ac_device_ptr->number_drives; i++) {
                  ac_device_ptr->ac_drive_table[i].inuse = 0;
               }
               for (i=0; i<ac_device_ptr->number_drives; i++) {
                  if ((ac_device_ptr->ac_drive_table[i].opened != 0) && 
                      (ac_device_ptr->ac_drive_table[i].surface != EMPTY)) {
                     /* close the drives that were opened */
                     drive_ptr = &(ac_device_ptr->ac_drive_table[i]);
#ifndef _WSIO
                     (*cdevsw[drive_maj_char].d_close)(makedev(drive_maj_char,
                        makeminor(0,drive_ptr->logical_unit,0,2)));
#else
                     (*cdevsw[drive_maj_char].d_close)(makedev(drive_maj_char,
                        ac_minor(ac_card(ac_device_ptr->device),
                        drive_ptr->scsi_address,0,0)),0);
#endif
                     if (AC_DEBUG & OPERATION)
                        msg_printf0("AUTOCH_CLOSE: Final close of drives\n");
                     drive_ptr->opened = 0;
                  }
               }
            }       
         case 5: /* nothing new */
         case 4: /* nothing new */
         case 3: /* nothing new */
         case 2: 
            if (total_open_cnt(ac_device_ptr) == 0) {
               release_drives(ac_device_ptr);
               autox0_close(ac_dev);
               ac_device_ptr->opened = 0;
            }
         case 1: 
            /* wakeup anybody waiting to open/close */
            if (ac_device_ptr->ocl & (WANT_OPEN | WANT_CLOSE)) {
               if (AC_DEBUG & OPEN_CLOSE)
                  msg_printf1("%d   autoch_open: waking up. surface %d\n", u.u_procp->p_pid, surface);
               wakeup((caddr_t)&(ac_device_ptr->ocl));
            }
            if (ac_open_init & INITIALIZING_WANTED) {
               wakeup((caddr_t)&ac_open_init);
            }
         
            ac_device_ptr->ocl = 0;
            if (error_switch == 1) {
               /* only do this if case */
               DELETE_FROM_LIST(ac_device_head,ac_device_ptr);
               FREE_ENTRY(ac_device_ptr,ac_device);
            }

            break;

         default:
            panic("Bad error recovery in autoch_open");
      }
      return(escapecode);
   }
}

/*
 * This routine calls the disk driver ioctl to do an xsense
 * on a device.
 */

int
autoch_sioc_xsense(dev, v, drive_ptr)
   dev_t dev;
   caddr_t v;
   struct ac_drive *drive_ptr;
{
   int error = 0;

   if (AC_DEBUG & MP_IOCTL)
      msg_printf("autoch_sioc_xsense\n");

   error = (*cdevsw[drive_maj_char].d_ioctl)(dev, SIOC_XSENSE, v, 0);

   return (error);
} /* autoch_sioc_xsense */


/*
 * This sets an entire surface as exclusively opened.  This only works
 * on the entire surface . An exclusive open on a section other than 
 * the entire surface it returns
 * as invalid.  This routine is a in a critical section so the open
 * count does not get corrupted if there is another open on the same
 * device at the same time.
 * This exclusive is at the surface level and the the disk driver
 * gets set to exclusive when it is swapped into a drive.
 */

int
autoch_dioc_exclusive(dev, v, ac_device_ptr, flag)
   dev_t dev;
   caddr_t v;
   struct ac_device *ac_device_ptr;
   int flag;
{
   int surface;
   int section;
   int exclusive;
   int error = 0;
   int pri;

   if (AC_DEBUG & MP_IOCTL)
      msg_printf("autoch_dioc_exclusive\n");

   /* determine which surface and section */
   surface = ac_surface (dev);
   section = ac_section (dev); 

   pri = spl6(); /* critical section of exclusive for open count */

   if ((section == ENTIRE_SURFACE) && (ac_device_ptr->elements[surface][section] != NULL)) {
         if ((ac_device_ptr->elements[surface][section]->open_count == 1) && (flag & FWRITE)) {

             exclusive = *((int *)v);
             if (exclusive == 1 || exclusive == 0) 
                ac_device_ptr->elements[surface][section]->exclusive_lock = exclusive;
             else 
            error = EINVAL; /* not a valid parameter */

         }
         else error = EACCES; /* already opened so not able to open exclusively */
   }
   else error = EINVAL; /* invalid section */

   splx(pri); /* end critical section */
   return (error);

} /* autoch_dioc_exclusive */

/*
 * This routine sets the WOE flag in the section struct for a specific
 * surface device. The setting of the disk driver to WOE is set in
 * call_async_op and set_drive_mode if the surface is swapped out of
 * a drive when the WOE flag is set and then is swapped back in. 
 */

int
autoch_sioc_woe(dev, v, ac_device_ptr)
   dev_t dev;
   caddr_t v;
   struct ac_device *ac_device_ptr;
{
   int surface;
   int section;
   int woe;
   int error = 0;

   if (AC_DEBUG & MP_IOCTL)
      msg_printf("autoch_sioc_woe\n");

   /* determine which surface and section */
   surface = ac_surface (dev);
   section = ac_section (dev); 

   /* ioctl only on the entire surface... no section support */
   if (section == ENTIRE_SURFACE) {

      /* if the surface is exclusively locked */
      if (ac_device_ptr->elements[surface][section]->exclusive_lock == 1) {
   
         woe = *((int *)v);
         if (woe == 1)   /* turn write without erase on */
            ac_device_ptr->elements[surface][section]->section_flags |= S_WOE;
         else if (woe == 0)      /* turn write without erase off */
            ac_device_ptr->elements[surface][section]->section_flags &= ~S_WOE;
         else 
            error = EINVAL;  /* invalid woe parameter */

      }
      else error = EACCES;        /* exclusive lock not set */
   }
   else error = EINVAL;           /* invalid section set */

   return (error);

} /* autoch_sioc_woe */

/*
 * This routine sets the WWV flag in the section struct for a specific
 * surface device. The setting of the disk driver to WWV is set in
 * call_async_op and set_drive_mode if the surface is swapped out of
 * a drive when the WWV flag is set and then is swapped back in. 
 */

int
autoch_sioc_wwv(dev, v, ac_device_ptr)
   dev_t dev;
   caddr_t v;
   struct ac_device *ac_device_ptr;
{
   int surface;
   int section;
   int wwv;
   int error = 0;

   if (AC_DEBUG & MP_IOCTL)
      msg_printf("autoch_sioc_wwv\n");

   /* determine which surface and section */
   surface = ac_surface (dev);
   section = ac_section (dev); 
   
   /* ioctl only on the entire surface... no section support */
   if (section == ENTIRE_SURFACE) {

         wwv = *((int *)v);
         if (wwv == 1)       /* turn write with verify on */
            ac_device_ptr->elements[surface][section]->section_flags |= S_WWV;
         else if (wwv == 0)  /* turn write with verify off */
            ac_device_ptr->elements[surface][section]->section_flags &= ~S_WWV;
         else 
            error = EINVAL;  /* invalid write with verify parameter */

   }
   else error = EINVAL;      /* section is not valid */

   return (error);

} /* autoch_sioc_wwv */

/*
 * This routine will do an erase on a surface based on the erase struct.
 * First it checks to see that the device has been exclusively opened.
 * Then it sets exclusive in the disk driver and calls the erase ioctl
 * to perform the erase.
 */

int
autoch_sioc_erase(dev, v, drive_ptr)
   dev_t dev;
   struct scsi_erase *v;
   struct ac_drive *drive_ptr;
{
   int section;
   int error = 0;
   int flag = 1;

   if (AC_DEBUG & MP_IOCTL)
      msg_printf("autoch_sioc_erase\n");

#ifndef _WSIO
   section = drive_section(dev);
#else
   section = ac_section(dev);
#endif

   /* ioctl only on the entire surface... no section support */
   if (section == ENTIRE_SURFACE) {

      if (drive_ptr->ac_device_ptr->elements[drive_ptr->surface][section]->exclusive_lock == 1) {
         if ((error = (*cdevsw[drive_maj_char].d_ioctl)(dev, DIOC_EXCLUSIVE, &flag, FWRITE)) == 0) {
            error = (*cdevsw[drive_maj_char].d_ioctl)(dev, SIOC_ERASE, v, FWRITE);
         } /* if DIOC_EXCLUSIVE */
      }
      else error = EACCES; /* not exclusively locked */
   }
   else error = EINVAL; /* not a valid section */

   return (error);

} /* autoch_sioc_erase */

/*
 * This routine calls the disk driver ioctl to do a verify
 * (to verify a written sector) on a device.
 */
int
autoch_sioc_verify(dev,v,drive_ptr)
   dev_t dev;
   struct ioctlarg *v;
   struct ac_drive *drive_ptr;
{
   int error = 0;
   int flag  = 1;
   int section;

   if (AC_DEBUG & MP_IOCTL)
    msg_printf("autoch_sioc_verify\n");

#ifndef _WSIO
   section = drive_section(dev);
#else
   section = ac_section(dev);
#endif

   /* ioctl only on the entire surface ... no section support */
   if (section == ENTIRE_SURFACE) {
      error = (*cdevsw[drive_maj_char].d_ioctl)(dev,SIOC_VERIFY,v,0);
   }
   else error = EINVAL; /* not a valid section */

   return (error);
} /* autoch_sioc_verify */

/*
 * This routine calls the disk driver ioctl to do a verify blank
 * on a device.
 */
int
autoch_sioc_verify_blank(dev,v,drive_ptr)
  dev_t dev;
   struct ioctlarg *v;
   struct ac_drive *drive_ptr;
{
   int error = 0;
   int flag  = 1;
   int section;

   if (AC_DEBUG & MP_IOCTL)
    msg_printf("autoch_sioc_verify_blank\n");

#ifndef _WSIO
   section = drive_section(dev);
#else
   section = ac_section(dev);
#endif

   /* ioctl only on the entire surface ... no section support */
   if (section == ENTIRE_SURFACE) {
      error = (*cdevsw[drive_maj_char].d_ioctl)(dev,SIOC_VERIFY_BLANK,v,0);
   }
   else error = EINVAL; /* not a valid section */

   return (error);
} /* autoch_sioc_verify_blank */

/*
 * This routine sets the IMMED flag in the section struct for a specific
 * surface device. The setting of the drive to IMMED is set in
 * call_async_op and set_drive_mode if the surface is swapped out of
 * a drive when the IMMED flag is set and then is swapped back in.
 */
int
autoch_set_immed(dev, v, ac_device_ptr)
   dev_t dev;
   caddr_t v;
   struct ac_device *ac_device_ptr;
{
   int surface;
   int immed;
   int error = 0;

   if (AC_DEBUG & MP_IOCTL)
      msg_printf("autoch_set_immed\n");

   /* determine which surface and section */
   surface = ac_surface (dev);

   immed = *((int *)v);
   if (immed == 1)        /* turn immediate reporting on */
      ac_device_ptr->phys_drv_state[surface] |= S_IMMED;
   else if (immed == 0)   /* turn immediate reporting off */
      ac_device_ptr->phys_drv_state[surface] &= ~S_IMMED;
   else
      error = EINVAL;  /* invalid immed parameter */

   return (error);
} /* autoch_set_immed */

/*
 * This routine reads the status of the immediate reporting parameter.
 * If immediate reporting is enabled (on) write requests may be
 * acknowledged before the data is physically transferred to the media.
 * Disabling immediate report forces the device to await completion of
 * any write request before reporting its status.
 * This routine checks the phys_drv_state flag in the autochanger driver
 * instead of calling the scsi ioctl sioc_get_ir.
 * Since this does not require a device access, this routine is called
 * directly from autoch_ioctl(), not through process_op().
 */
int
autoch_sioc_get_ir(ac_dev_ptr,v,surface)
   struct ac_device  *ac_dev_ptr;
   caddr_t           v;
   int               surface;
{
   int error = 0;   /* this function will never fail */

   *v = (char)(ac_dev_ptr->phys_drv_state[surface] & S_IMMED);
   return(error);

} /* autoch_sioc_get_ir */

/*
 * Set the immediate reporting parameter on or off.
 * If immediate reporting is enabled (on) write requests may be
 * acknowledged before the data is physically transferred to the media.
 * Disabling immediate report forces the device to await completion of
 * any write request before reporting its status.
 * This routine calls the sioc_set_ir ioctl through the disk driver and
 * is called from process op.
 */
int
autoch_sioc_set_ir(dev,v,drive_ptr)
   dev_t dev;
   caddr_t v;
   struct ac_drive *drive_ptr;
{
   int error = 0;

   error = (*cdevsw[drive_maj_char].d_ioctl)(dev, SIOC_SET_IR, v, 0);
   return (error);

} /* autoch_sioc_set_ir */

#ifndef _WSIO
/*
 * This routine flushes the internal write cache to the physical medium.
 * Used by devices in immediate reporting mode to flush internal buffers.
 * This routine calls the sioc_sync_cache ioctl through the disk driver
 * and is called from process op.
 */
int
autoch_sioc_sync_cache(dev,v,drive_ptr)
   dev_t dev;
   caddr_t v;
   struct ac_drive *drive_ptr;
{
   int error = 0;

   error = (*cdevsw[drive_maj_char].d_ioctl)(dev, SIOC_SYNC_CACHE, v, 0);
   return (error);

} /* autoch_sioc_sync_cache */
#endif /* ! _WSIO */

#ifdef PASS_THRU
/*
 * This routine will do a pass thru scsi command on a surface.
 * First it checks to see that the device has been exclusively opened.
 * Then it sets exclusive in the disk driver and calls the erase ioctl
 * to perform the erase.
 */

int
autoch_sioc_io(dev, v, drive_ptr)
   dev_t dev;
   struct sctl_io *v;
   struct ac_drive *drive_ptr;
{
   int section;
   int error = 0;
   int flag = 1;

   if (AC_DEBUG & MP_IOCTL)
      msg_printf("autoch_sioc_io\n");

#ifndef _WSIO
   section = drive_section(dev);
#else 
   section = ac_section(dev);
#endif 

   /* ioctl only on the entire surface... no section support */
   if (section == ENTIRE_SURFACE) {

      if (drive_ptr->ac_device_ptr->elements[drive_ptr->surface][section]->exclusive_lock == 1) {
         if ((error = (*cdevsw[drive_maj_char].d_ioctl)(dev, DIOC_EXCLUSIVE, &flag, FWRITE)) == 0) {
            error = (*cdevsw[drive_maj_char].d_ioctl)(dev, SIOC_IO, v, FWRITE);
         } /* if DIOC_EXCLUSIVE */
      }
      else error = EACCES; /* not exclusively locked */
   }
   else error = EACCES; /* not a valid section */

   return (error);

} /* autoch_sioc_io */

#endif /* PASS_THRU */

/*
 * This routine calls the sioc_inquiry through the disk driver and
 * is called from process op.
 */ 

int
autoch_sioc_inquiry(dev, v, drive_ptr)
   dev_t dev;
   caddr_t v;
   struct ac_drive *drive_ptr;
{
   int error = 0;

   if (AC_DEBUG & MP_IOCTL)
      msg_printf("autoch_sioc_inquiry\n");

   error = (*cdevsw[drive_maj_char].d_ioctl)(dev, SIOC_INQUIRY, v, 0);

   return (error);

} /* autoch_sioc_inquiry */

/*
 * This routine calls the sioc_capacity through the disk driver and
 * is called from process op.
 */ 

#ifdef _WSIO
int
autoch_sioc_capacity(dev, v, drive_ptr)
   dev_t dev;
   caddr_t v;
   struct ac_drive *drive_ptr;
{
   int error = 0;

   if (AC_DEBUG & MP_IOCTL)
      msg_printf("autoch_sioc_capacity\n");

   error = (*cdevsw[drive_maj_char].d_ioctl)(dev, SIOC_CAPACITY, v, 0);

   return (error);

} /* autoch_sioc_capacity */
#endif /* _WSIO */

/*
 * This routine calls the dioc_describe through the disk driver and
 * is called from process op.
 */ 

int
autoch_disk_describe(dev,v,drive_ptr,code)
   dev_t dev;
   caddr_t v;
   struct ac_drive *drive_ptr;
   int code;
{
   int error = 0;

   error = (*cdevsw[drive_maj_char].d_ioctl)(dev, code, v, 0);
   return (error);

} /* autoch_disk_describe */


/*
 * This routine calls the dioc_capacity through the disk driver and
 * is called from process op.
 */ 

int
autoch_disk_capacity(dev,v,drive_ptr)
   dev_t dev;
   caddr_t v;
   struct ac_drive *drive_ptr;
{
   int error = 0;

   error = (*cdevsw[drive_maj_char].d_ioctl)(dev, DIOC_CAPACITY, v, 0);
   return (error);

} /* autoch_disk_capacity */


/*
 * In order to format the media call the appropriate SCSI
 * ioctls.
 * When this is called, the surface is already in the drive.
 */
int
autoch_format(dev,v,drive_ptr)
   dev_t dev;
   caddr_t v;
   struct ac_drive *drive_ptr;
{
   int error = 0;
   int mode;
#ifdef _WSIO
   struct sioc_format scsi_format;
#endif

   /* Put SCSI driver into special command mode */
   mode = 1;
   if ((error=(*cdevsw[drive_maj_char].d_ioctl)(dev, DIOC_EXCLUSIVE, &mode, FWRITE))!=0) {
      return(error);
   }

#ifndef _WSIO

   drive_ptr->interleave = 0;

   if ((error = (*cdevsw[drive_maj_char].d_ioctl)(dev, SIOC_FORMAT, &(drive_ptr->interleave), FWRITE)) != 0) {
      mode = 0;       
      if ((*cdevsw[drive_maj_char].d_ioctl)(dev, DIOC_EXCLUSIVE, &mode, FWRITE) != 0) {
         printf("AUTOCH: could not clear DIOC_EXCLUSIVE\n");
      }
      return(error);
   }

#else

   scsi_format.fmt_optn = 0;
   scsi_format.interleave = *((short *)v);

   if ((error = (*cdevsw[drive_maj_char].d_ioctl)(dev, SIOC_FORMAT, &scsi_format, FWRITE)) != 0) {
      /* format failed turn off exclusive and return */
      mode = 0;
      if ((*cdevsw[drive_maj_char].d_ioctl)(dev, DIOC_EXCLUSIVE, &mode, FWRITE) != 0) {
         printf("AUTOCH: could not clear DIOC_EXCLUSIVE\n");
      }
      return(error);
   }

#endif

   mode = 0;       
   if ((error=(*cdevsw[drive_maj_char].d_ioctl)(dev, DIOC_EXCLUSIVE, &mode, FWRITE)) != 0) {
      return(error);
   }

   return (error);
}

/*
 * Close the surface by moving the media home.
 * When this is called, the surface is already in the drive.
 */
int
close_surface(ac_device_ptr,drive_ptr)
   struct ac_device *ac_device_ptr;
   struct ac_drive *drive_ptr;
{
 
   int error=0;
   struct xport_request *xport_reqp;
   int pri;

   /* move it home.  The xport daemon will take care of the 
      physical unmounting of the surface. */
   drive_ptr->drive_state = DAEMON_CALL;
   ac_device_ptr->transport_busy = 1;

   xport_reqp = init_xport_request();
   /* Put in the move parameters */
   xport_reqp->move_request.transport = ac_device_ptr->transport_address;
   xport_reqp->move_request.source = drive_ptr->element_address;
   xport_reqp->move_request.destination= SURTOEL(drive_ptr->surface,
                  ac_device_ptr->storage_element_offset);
   xport_reqp->x_flags |= X_CLOSE_SURFACE;
   xport_reqp->move_request.invert = FLIP(drive_ptr->surface);
   if (AC_DEBUG & QUEUES) {
      msg_printf0("XPORT");
      print_xport_queue(ac_device_ptr->xport_queue_head);
   }
   ADD_TO_LIST(ac_device_ptr->xport_queue_head,xport_reqp);
   if (AC_DEBUG & QUEUES) 
      print_xport_queue(ac_device_ptr->xport_queue_head);

   pri = spl6();
   wakeup(&(ac_device_ptr->xport_queue_head));
   sleep(xport_reqp,PRIBIO);
   (void) splx(pri);

   /* check for error */
   if (!(xport_reqp->x_flags & X_DONE)) { /* move had an error */ 
      error = xport_reqp->error;
      if(!error) 
         panic("close_surface: Failed to complete but gave no error");
   }
   FREE_ENTRY(xport_reqp,xport_request);
   return(error);
}


autoch_close(dev)
   dev_t dev;
{
   int i,surface, element,section,pri;
   struct operation_parms op_parm;
   struct ac_device *ac_device_ptr;
   struct ac_drive *drive_ptr = NULL;
   extern struct surface_request *add_request();
   extern struct ac_device *ac_opened ();
   extern void wait_time_done ();
   extern void autox0_close();
   extern void punmount_section();
   extern void punmount_surface();

   if (ON_ICS)
   {
       panic("autoch_close: called ON_ICS");
   }

   if ((ac_device_ptr = ac_opened(dev)) == NULL) {
      return(ENXIO);
   }

   /* check global open lock (initializing)            */
   /* check to see if autochanger header has ever been allocated, then     */
   /* check the lock flag...order of evaluation IS important          */
   /* synchronize with other open/close routines            */
   /* NOTE: if this sleeps because of INITIALIZING, it will be woken       */
   /*         up because the routine INITIALIZING will eventually go to     */
   /*         IN_OPEN and wake it up               */

   /* determine which surface */
   surface = ac_surface (dev);

   if (AC_DEBUG & OPEN_CLOSE)
      msg_printf1("%d   autoch_close: closing. surface %x\n", u.u_procp->p_pid, surface);
   while ((ac_open_init & INITIALIZING) ||
          (ac_device_ptr->ocl & (IN_CLOSE | IN_OPEN))) {

      /* you are locked out, please wait */
      ac_device_ptr->ocl |= WANT_CLOSE;
      if (AC_DEBUG & OPEN_CLOSE)
         msg_printf1("%d   autoch_close: sleeping. surface %x\n", u.u_procp->p_pid, surface);
      sleep(&(ac_device_ptr->ocl),PLOCK );
   }

   ac_device_ptr->ocl |= IN_CLOSE;
   if (AC_DEBUG & OPEN_CLOSE)
      msg_printf1("%d   autoch_close: closed. surface %x\n", u.u_procp->p_pid, surface);

   /* determine which section */
   section = ac_section(dev);
   if (surface == 0)
      section = 0;
   if ((section == 10) || (section == 11)) {
      return(EINVAL);
   }

   if (ac_device_ptr->jammed) {
      if (ac_send_reset(ac_device_ptr) != 0) {
         /* autochanger still jammed, don't allow use */
         /* decrement the open count for this surface */
         ac_device_ptr->elements[surface][section]->open_count -= 1;
         goto end_close;
      }
   }

   /* get element number */
   element = SURTOEL(surface,ac_device_ptr->storage_element_offset);

   pri = spl6();
#ifndef _WSIO
   /* Only get a close on the final close of the device. */
   /* The number of opens do not match the number of closes. */
   /* Just set the open count to zero */
   /* Except for the autochanger, that needs to be set back to 2 */
   /* for the daemons. */
   if (surface != 0) {

      /* Check to see if there is a character device and a block device 
       * open for this surface.  If both are open turn off the appropriate 
       * flag and return grace fully and reduce the open count by one.
       */

      if ((ac_device_ptr->elements[surface][section]->section_flags & AC_BLK_OPEN) &&
          (ac_device_ptr->elements[surface][section]->section_flags & AC_CHAR_OPEN)) { 
          if (major(dev) == AC_MAJ_BLK)
             ac_device_ptr->elements[surface][section]->section_flags &= ~AC_BLK_OPEN;
          else
             ac_device_ptr->elements[surface][section]->section_flags &= ~AC_CHAR_OPEN;

          ac_device_ptr->elements[surface][section]->open_count -= 1;
      }
      else {

          /* Only one open is set, so clear the appropriate flag and set the open count */
          /* to zero */

          if (ac_device_ptr->elements[surface][section]->section_flags & AC_BLK_OPEN) 
             ac_device_ptr->elements[surface][section]->section_flags &= ~AC_BLK_OPEN;
          else
             ac_device_ptr->elements[surface][section]->section_flags &= ~AC_CHAR_OPEN;

          ac_device_ptr->elements[surface][section]->open_count = 0;
      }
   }
   else {  /* keep the open count at 2 for the picker device */
      ac_device_ptr->elements[surface][section]->open_count = 2;
   } 
#else /* s300 and s700 */
   /* decrement the open count for this surface */
   (ac_device_ptr->elements[surface][section]->open_count)--;
#endif
   splx(pri);

   /* is this the last close for this section? */
   if (ac_device_ptr->elements[surface][section]->open_count == 0) {
      if (current_sur_inserted(ac_device_ptr,surface,&drive_ptr)) {
         /* make sure this section gets closed. */
         /* make sure WOE and WWV are cleared if needed */
         /* punmount_section will make sure the spinup section doesn't get unmounted */
         punmount_section(drive_ptr,section,NULL);
      }
      /* clear all the section flags and exclusive lock for the last close on the device */
      ac_device_ptr->elements[surface][section]->section_flags = 0;
      ac_device_ptr->elements[surface][section]->exclusive_lock = 0;

      kmem_free ((caddr_t)(ac_device_ptr->elements[surface][section]),
         (u_int)(sizeof(struct section_struct))); 
      ac_device_ptr->elements[surface][section] = NULL;
   }

   /* is this the last close for this surface? */
   if (total_surface_count(ac_device_ptr,surface) != 0) {
      goto end_close ;
   }

   /* Do the last close on the surface */
   if (current_sur_inserted(ac_device_ptr,surface,&drive_ptr) ||
      same_sur_in_q(surface,ac_device_ptr)) {
      process_op(ac_device_ptr,close_surface,&op_parm,surface,
            "in close_surface");

      /* Cancel wait_time_done, because there is no more work to do. */
      if ((total_open_cnt(ac_device_ptr) == 3) &&  /* assume the daemons are running */
         (drive_ptr->drive_state == WAIT_TIME)) {
         untimeout (wait_time_done, (caddr_t) drive_ptr);
      }
   }


   /* release element on last close of surface */
   if (surface != 0) {
      /* is this the last usage of all surfaces on element?? */
      if (total_element_count(ac_device_ptr, element) == 0) {
         /* if so, release element so user has access to it */
         release_element(ac_device_ptr, element);
      }
   }

   if ((ac_device_ptr->elements[0][0] == NULL) ||
      (total_open_cnt(ac_device_ptr) == ac_device_ptr->elements[0][0]->open_count)) {
      /* only ioctl's have things open, so release drives */
      release_drives(ac_device_ptr);
      for (i=0; i<ac_device_ptr->number_drives; i++) {
         ac_device_ptr->ac_drive_table[i].inuse = 0;
      }
   }

   if (total_open_cnt(ac_device_ptr) <= 0) {

      /* stuff to do on final close */
   
      /* close the drives that have cartridges in them */
      for (i = 0; i < ac_device_ptr->number_drives; i++) {
         if (ac_device_ptr->ac_drive_table[i].surface != EMPTY) {
            drive_ptr = &(ac_device_ptr->ac_drive_table[i]);
            punmount_surface(drive_ptr,NULL);
            if (AC_DEBUG & OPERATION)
               msg_printf0("AUTOCH_CLOSE: Final close of drives\n");
            drive_ptr->opened = 0;
            drive_ptr->drive_state = NOT_BUSY;
         }
      }
      /* close the mechanical autochanger */
      ac_device_ptr->opened = 0;
      /* scsi_close can't return an error */
      autox0_close(ac_device_ptr->device);
   }

end_close:

   
   /* wakeup anybody waiting to open/close */
   if (ac_device_ptr->ocl & (WANT_OPEN | WANT_CLOSE)) {
      if (AC_DEBUG & OPEN_CLOSE)
         msg_printf1("%d   autoch_close: waking up. surface %x\n", u.u_procp->p_pid, surface);

      wakeup((caddr_t)&(ac_device_ptr->ocl));
   }
   if (ac_open_init & INITIALIZING_WANTED) {
      wakeup((caddr_t)&ac_open_init);
   }

   ac_device_ptr->ocl = 0;

   return(0) ;
}

autoch_read(dev,uio)
   dev_t dev;
   struct uio *uio;
{
   int surface, error = 0;
   int page_code = ALL_PC;
   struct operation_parms op_parm;
   struct ac_device *ac_device_ptr;
   extern struct ac_device *ac_opened ();

   if ((ac_device_ptr = ac_opened(dev)) == NULL) {
      panic ("autoch_read: unopened autochanger");
   }

   /* determine which surface */
   surface = ac_surface (dev);

   /* can't do io on surface zero */
   if (surface == 0) {
      if (ac_device_ptr->read_element_status_flag) {
         if ((error = autox0_ioctl(ac_device_ptr->device,AUTOX0_READ_ELEMENT_STATUS,&page_code)) == 0) {
            error = autox0_read(ac_device_ptr->device,uio);
         }
         /* reset flag */
         ac_device_ptr->read_element_status_flag = 0;

         return(error);
      } else {
         return(ENXIO);
      }
   }

   /* load up the operation parameter structure */
   op_parm.dev = preprocess_dev (dev);
   op_parm.uio = uio;

   /* call the operation */
   error = process_op (ac_device_ptr, cdevsw[drive_maj_char].d_read, &op_parm, surface, "in scsi_read");

   /* unload the operation parameters (dev remains with surface info) */
   uio = op_parm.uio;
   if (error) {
      msg_printf1("Error = %d\n",error);
      /* panic("AC_READ:process_op error"); */
   }

   return (error);
}


autoch_write(dev,uio)
   dev_t dev;
   struct uio *uio;
{
   int surface, error;
   struct operation_parms op_parm;
   struct ac_device *ac_device_ptr;
   extern struct ac_device *ac_opened ();

   if ((ac_device_ptr = ac_opened(dev)) == NULL) {
      panic ("autoch_write: unopened autochanger");
   }

   /* determine which surface */
   surface = ac_surface (dev);

   /* can't do io on surface zero */
   if (surface == 0) {
      return(ENXIO);
   }

   /* load up the operation parameter structure */
   op_parm.dev = preprocess_dev (dev);
   op_parm.uio = uio;
   op_parm.flag = dev;

   /* call the operation */
   error = process_op (ac_device_ptr, cdevsw[drive_maj_char].d_write, &op_parm, surface, "in scsi_write");

   /* unload the operation parameters (dev remains with surface info) */
   uio = op_parm.uio;

   return (error);
}


/*****************************************************************************
 *                store_b_dev
 * Takes the dev out of the bp->b_dev and stores it in a linked list for
 * later restoration.
 */
void
store_b_dev(bp)
    struct buf *bp;
{
    struct dev_store *dev_store_ptr;
    int s;
    extern int restore_b_dev();

    /* allocate some memory */
    dev_store_ptr = (struct dev_store *)kmem_alloc(sizeof(struct dev_store));

    if (dev_store_ptr == NULL) {
        panic("Error with kmem_alloc in store_dev().");
    }

    /* added structure to list */
    ADD_TO_LIST(dev_store_head, dev_store_ptr);

    s = spl6();
    /* copy info into structure */
    dev_store_ptr->dev        = bp->b_dev;
    dev_store_ptr->B_CALL_set = ( (bp->b_flags & B_CALL) == B_CALL);
    dev_store_ptr->b_iodone   = bp->b_iodone;
    dev_store_ptr->bp         = bp;

    bp->b_flags |= B_CALL;        /* so iodone calls our routine */    
    bp->b_iodone = restore_b_dev;    /* our routine */
    splx(s);

    /*msg_printf("dev_store alloc'd:bp=%x dev=%x\n",bp,dev_store_ptr->dev); */

    return;
}

/*****************************************************************************
 *                restore_b_dev
 * Finds the dev on the linked list associated with the bp and returns it.
 */
int
restore_b_dev(bp)
    struct buf *bp;
{
    struct dev_store *dev_store_ptr;
    int dev;
       int s;

    /* find the matching bp */

  s = spl6();      /* keep out all interrupts that might modify the list */

    /* loop thru the queue */
    dev_store_ptr=dev_store_head; 
    while (dev_store_ptr != NULL) { 
    if (dev_store_ptr->bp == bp) {   /* this is it */
        break;
    }
    dev_store_ptr = dev_store_ptr->forw;
    }

    if (dev_store_ptr == NULL) {
    /* this means bp not in list - very BAD */
       printf("matching bp %x not found in dev_store list", bp);
       panic("matching bp not found in dev_store list");
    }

    /* copy info back into bp */
    bp->b_dev      = dev_store_ptr->dev;
    bp->b_iodone = dev_store_ptr->b_iodone;

    if (dev_store_ptr->B_CALL_set)  bp->b_flags |=  B_CALL;
    else                            bp->b_flags &= ~B_CALL;

    bp->b_flags &= ~B_DONE;    /* have to do to call iodone again */

  splx(s);

    iodone(bp);

    /* delete from list */
    DELETE_FROM_LIST(dev_store_head, dev_store_ptr);

    /*msg_printf("dev_store del'd:bp=%x dev=%x\n", bp, dev_store_ptr->dev); */

    /* deallocate memory */
    FREE_ENTRY(dev_store_ptr, dev_store);

    return(0);
}


autoch_strategy(bp)
   struct buf *bp;
{
   int surface;
   int error;
   struct ac_device *ac_device_ptr;
   struct operation_parms op_parm;   /* problem */
   extern struct ac_device *ac_opened ();
   void store_b_dev();

   if (ON_ICS)
   {
       panic("autoch_strategy: called ON_ICS");
   }

   /* because strategy could be called more than once for a given
    * buffer structure this routine needs to check for this condition
    * and modify the surface and dev appropriately.
    */

   /* msg_printf1("AC STRAT CALL buf = %x\n",bp);*/ 
   if ((major(bp->b_dev) == AC_MAJ_BLK) || (major(bp->b_dev) == AC_MAJ_CHAR)) {
      /* first time through */
      if ((ac_device_ptr = ac_opened(bp->b_dev)) == NULL) {
         panic ("autoch_strategy: unopened autochanger");
      }

      /* determine which surface */
      surface = ac_surface(bp->b_dev);

      /* store the dev for later use if multiple passes
       * for same buffer 
       */
      store_b_dev(bp);
      
      /* put the correct dev into buffer */
      bp->b_dev = preprocess_dev (bp->b_dev);

   } else {  /* not the first time through */

      panic("AUTOCHANGER: bad path in autoch_strategy");
   }

   /* load up the operation parameter structure */
   op_parm.bp = bp;

   /* call the operation */
   error = process_op (ac_device_ptr, bdevsw[major(bp->b_dev)].d_strategy, &op_parm, surface, "in scsi_strategy");
   if (error) {
      bp->b_error = error;
      bp->b_flags |= B_ERROR;
      iodone(bp);
   }
}

int
autoch_ioctl(dev,code,v,flag)
   dev_t dev;
   int code;
   caddr_t v;
   int flag;
{
   int surface, error;
   struct operation_parms op_parm;
   struct ac_device *ac_device_ptr;
   extern struct ac_device *ac_opened ();

   int autoch_set_wwv_mode(); /* sets the disk driver to verify writes */
   int autoch_set_woe_mode(); /* sets the disk driver to write without erase */
   int autoch_sioc_set_ir();  /* sets the drive to do immed reporting */

   if ((ac_device_ptr = ac_opened(dev)) == NULL) {
      panic ("autoch_ioctl: unopened autochanger");
   }

   if (ac_device_ptr->jammed) {
      if (ac_send_reset(ac_device_ptr) != 0) {
         /* autochanger still jammed, don't allow use */
         return(EIO);
      }
   }

   /* determine which surface */
   surface = ac_surface(dev);

   /* ioctl's only allowed on autochanger and on surfaces.
    * ioctl's not allowed on block devices except ACIOC_FORMAT
    */
   if (major(dev) == drive_maj_blk) {
      return(ENXIO);
   }
   if (surface != 0) {

      /* load up the parameters for the ioctl's parameter structure */
      op_parm.dev = preprocess_dev (dev);
      op_parm.code = code;
      op_parm.v = v;

      if ((code & ~IOCSIZE_MASK) == (DIOC_DESCRIBE & ~IOCSIZE_MASK)) {
         /* it's a request for describe, with either the old length or    */
         /* the new length; scsi driver will pass back the correct amount */
         error = process_op(ac_device_ptr, autoch_disk_describe, &op_parm, surface, "in autoch_disk_describe");
         return(error);

      } else {
         switch (code) {

            case ACIOC_READ_Q_CONST:
              op_parm.dev = dev;
              error = do_ioctl (&op_parm, ac_device_ptr);
              return(error);
              break;

            case ACIOC_WRITE_Q_CONST:
              op_parm.dev = dev;
              error = do_ioctl (&op_parm, ac_device_ptr);
              return(error);
              break;

            case ACIOC_FORMAT:
               error = process_op (ac_device_ptr, autoch_format, &op_parm, surface, "in autoch_format");
               return(error);
               break;

            case DIOC_CAPACITY:
               error = process_op(ac_device_ptr, autoch_disk_capacity, &op_parm, surface, "in autoch_disk_capacity");
               return(error);
               break;

            case SIOC_XSENSE:
               error = process_op(ac_device_ptr, autoch_sioc_xsense, &op_parm, surface, "in autoch_sioc_xsense");
               return(error);
               break;

            case SIOC_INQUIRY:
               error = process_op(ac_device_ptr, autoch_sioc_inquiry, &op_parm, surface, "in autoch_sioc_inquiry");
               return(error);
               break;

#ifdef _WSIO
            case SIOC_CAPACITY:
               error = process_op(ac_device_ptr, autoch_sioc_capacity, &op_parm, surface, "in autoch_sioc_capacity");
               return(error);
               break;
#endif /* _WSIO */

            case DIOC_EXCLUSIVE:
               error = autoch_dioc_exclusive(dev, v, ac_device_ptr, flag);
               return(error);
               break;

            case SIOC_ERASE:
               error = process_op(ac_device_ptr, autoch_sioc_erase, &op_parm, surface, "in autoch_sioc_erase");
               return(error);
               break;

            case SIOC_WRITE_WOE:
               /* 
                * first set the woe flag at the autoch level and then
                * call process op to set the woe flag at the disk driver
                * level when the surface get put in the drive. 
                */
               if ((error = autoch_sioc_woe(dev, v, ac_device_ptr)) == 0) 
                  error = process_op(ac_device_ptr, autoch_set_woe_mode, &op_parm, surface, "in autoch_set_write_mode"); 
               return(error);
               break;

            case SIOC_VERIFY_WRITES:
               /* 
                * first set the wwv flag at the autoch level and then
                * call process op to set the wwv flag at the disk driver
                * level when the surface get put in the drive. 
                */
               if ((error = autoch_sioc_wwv(dev, v, ac_device_ptr)) == 0)
                  error = process_op(ac_device_ptr, autoch_set_wwv_mode, &op_parm, surface, "in autoch_set_write_mode"); 
               return(error);
               break;

            case SIOC_VERIFY:
               error = process_op(ac_device_ptr, autoch_sioc_verify, &op_parm, surface, "in autoch_sioc_verify");
               return (error);
               break;
   
            case SIOC_VERIFY_BLANK:
               error = process_op(ac_device_ptr, autoch_sioc_verify_blank, &op_parm, surface, "in autoch_sioc_verify_blank");
               return (error);
               break;

            case SIOC_GET_IR:
               error = autoch_sioc_get_ir(ac_device_ptr, v, surface);
               return(error);
               break;

            case SIOC_SET_IR:
               /*
                * first set the immed flag at the autoch level and then
                * call process op to set the immed flag at the disk driver
                * level when the surface gets put in the drive.
                * clear the flag if autoch_sioc_set_ir fails.
                */
               if ((error = autoch_set_immed(dev, v, ac_device_ptr)) == 0) {
                  error = process_op(ac_device_ptr, autoch_sioc_set_ir, &op_parm, surface, "in autoch_sioc_set_ir");
                  if (error) {
                     flag = 0;
                     autoch_set_immed(dev, &flag, ac_device_ptr);
                  }
               }
               return(error);
               break;

#ifndef _WSIO
            case SIOC_SYNC_CACHE:
               error = process_op(ac_device_ptr, autoch_sioc_sync_cache, &op_parm, surface, "in autoch_sioc_sync_cache");
               return(error);
               break;
#endif /* ! _WSIO */

#ifdef PASS_THRU

            case SIOC_IO:
               error = process_op(ac_device_ptr, autoch_sioc_io, &op_parm, surface, "in autoch_sioc_io");
               return(error);
               break;
   
#endif /* PASS_THRU */

            default: 
               return(ENXIO);
        } /* end switch code */
      } /* end if-else */
   } 
   else { /* surface == 0 */
      /* load up the operation parameter structure */
      op_parm.dev = ac_device_ptr->device;
      op_parm.code = code;
      op_parm.v = v;

      /* just call do_ioctl, don't need to call process op */
      error = do_ioctl (&op_parm, ac_device_ptr);

      /* unload the operation parameters */
      code = op_parm.code;
      v = op_parm.v;

      return (error);
   }
}

/*
 * This procedure takes a dev and returns a dev with the proper SCSI
 * select code and major number so that it can be passed down to the SCSI
 * driver.
 * Other bits are zeroed.
 *  
** When a request is queued for the 800s, the drive that will handle the request 
** is not known so the lu number is cleared.  The routine call_asyn_op sets the 
** lu number to the proper drive once the drive has been assigned to handle the 
** request.
*/
dev_t
preprocess_dev (dev)
   dev_t dev;
{
   int major_num;
   if (major(dev) == AC_MAJ_CHAR) { /* character device */
      major_num = drive_maj_char;
   }
   else {        /* block device     */
      major_num = drive_maj_blk;
   }
#ifndef _WSIO
   return((dev_t)makedev(major_num,makeminor(0,0,0,ac_section(dev))));
#else
   return((dev_t)makedev(major_num,ac_minor(ac_card(dev),0,0,0)));
#endif
}

/*
 * Calls the driver read command and converts it to a buffer.
 * It is the calling procedures responsibility to allocate
 * and release the appropiate size buffer.
 * The bp must contain the dev_t of the autochanger.
 * The error is returned in the bp.
 */

void
ac_read_element_status(ac_device_ptr,bp,page_code)
   struct ac_device *ac_device_ptr;
   struct buf *bp;
   int page_code;
{

   int error = 0;
   struct uio   acuio;
   struct iovec aciov;
   register struct uio   *acuiop = &acuio;
   register struct iovec *aciovp = &aciov;

   error = autox0_ioctl(bp->b_dev,AUTOX0_READ_ELEMENT_STATUS,&page_code);

   while (error == ECONNRESET)  {
      rereserve(ac_device_ptr);
      error = autox0_ioctl(bp->b_dev,AUTOX0_READ_ELEMENT_STATUS,&page_code);
   }
   if (error == 0) {
      /* setup to do a driver read into the kernel */
      acuiop->uio_iov=aciovp;
      acuiop->uio_iovcnt=1;
      acuiop->uio_offset = 0;
      acuiop->uio_seg=UIOSEG_KERNEL;
      acuiop->uio_fpflags=0;
      acuiop->uio_resid = aciovp->iov_len = (int)bp->b_bcount;
      aciovp->iov_base = bp->b_un.b_addr;
   
      error = autox0_read(bp->b_dev,acuiop);
   }
   if (error) {
      bp->b_flags |= B_ERROR;
      bp->b_error = error;
   }
}



/*
 * This procedure checks to see if a surface is in an autochanger,
 * returning 0 if surface IS in ac.
 */
int
surface_in_ac (ac_device_ptr,surface)
   struct ac_device *ac_device_ptr;
   int surface;
{
   int starting_element_address, el_desc_offset, return_value;
   int element, el_desc_len, error = 0, page_code, j;
   int drive_count;
   struct buf *el_bp;
   unsigned char *status_info;
   unsigned char *storage_info;
   unsigned char *drive_info;
   unsigned char *drive_entry;
   int numb_elements;

/*
 * get element status atomically, to avoid syncronization problems
 * which can occur when the autochanger is in motion
 * -paulc 10/26/1989
 */
   el_bp      = geteblk(EL_STATUS_SZ);
   el_bp->b_dev    = ac_device_ptr->device;
   page_code       = ALL_PC;
   ac_read_element_status(ac_device_ptr,el_bp,page_code);
   if (error = geterror(el_bp)) {
      brelse(el_bp);
      return(error);
   }

   status_info = (unsigned char *)el_bp->b_un.b_addr;

   storage_info = find_status_page(status_info,STORAGE_PC);

   /* what element are we looking for */
   element = SURTOEL(surface, ac_device_ptr->storage_element_offset);

   /* get starting element_address */
   starting_element_address = ac_device_ptr->storage_element_offset;

   /* get element_descriptor length for use as offset */
   el_desc_len = MAKE_WORD(storage_info[2],storage_info[3]);

   /* note - ignoring storage_info[5] */
   numb_elements = MAKE_WORD(storage_info[6],storage_info[7]) / el_desc_len
                   + ac_device_ptr->storage_element_offset - 1;

   /* get the offset in the data for the particular element */
   el_desc_offset = PC_HDR_LEN 
          + (element - starting_element_address) * el_desc_len;

   if (element > numb_elements) {
      /* then exceeded range of available elements */
      return_value = ENXIO;
   }
   /* check if this element is full */
   else if (storage_info[el_desc_offset + 2] & FULL_MSK) {
      return_value = 0;
   } else {
      return_value = ENXIO;

      drive_info = find_status_page(status_info,DATA_TRANSFER_PC);
   
      /* get element_descriptor length for use as offset */
      el_desc_len = MAKE_WORD(drive_info[2],drive_info[3]);
   
      /* number of drives is the number of elements */
      /* which is the number of data bytes / length of desc -paulc */
      drive_count = (
            (drive_info[5] << 16) |
            (drive_info[6] << 8)  |
            drive_info[7]
            ) / el_desc_len;
   
      /* loop thru the element descriptors to get the other info */
      for (j = 0, drive_entry = &(drive_info[8]);
         j < drive_count;
         drive_entry+=el_desc_len, j++) {
   
         /* compare source element address */
         if (    element ==
            MAKE_WORD(drive_entry[10],drive_entry[11]) )
            return_value = 0;
      }
   
   }

#ifdef KERNEL_DEBUG_ONLY
   if (force_error.surface_in_ac) return_value = ENXIO;
#endif KERNEL_DEBUG_ONLY
   brelse(el_bp);
   return (return_value);
}



/*
 * This procedure gets the transport address. 
 * -paulc 10/26/1989 (modified)
 */
int
get_transport_address (ac_device_ptr,status_info)
   struct ac_device *ac_device_ptr;
   unsigned char *status_info;
{
   unsigned char   *transport_info;
   struct transport_element_desc {
      unsigned char   elem_address_msb,
            elem_address_lsb,
            elem_status,
            reservd;
      } *transport_desc;
   int     elements,current_element,i;

   transport_info = find_status_page(status_info,TRANSPORT_PC);

   ac_device_ptr->transport_address = 0x10000;     /* bigger than two bytes */

   elements = (    (transport_info[5] << 16) |
         (transport_info[6] << 8)  |
         transport_info[7]
         ) / sizeof (struct transport_element_desc);

   transport_desc = (struct transport_element_desc *)&(transport_info[8]);
   for (i = 0; i < elements; i++,transport_desc++)
      {
         current_element = MAKE_WORD(    transport_desc->elem_address_msb,
                     transport_desc->elem_address_lsb);

         if (ac_device_ptr->transport_address > current_element)
            ac_device_ptr->transport_address = current_element;
      }
}



/*
 * This procedure gets the storage element offset.
 * -paulc 10/26/1989 (modified)
 */
int
get_storage_element_offset (ac_device_ptr,status_info)
   struct ac_device *ac_device_ptr;
   unsigned char *status_info;
{
   unsigned char   *storage_info;
   struct storage_element_desc {
      unsigned char   elem_address_msb,
            elem_address_lsb,
            elem_status,
            reservd;
      } *storage_desc;
   int     elements,current_element,i;

   storage_info = find_status_page(status_info,STORAGE_PC);

   ac_device_ptr->storage_element_offset = 0x10000;   /* bigger than two bytes */

   elements = (    (storage_info[5] << 16) |
         (storage_info[6] << 8)  |
         storage_info[7]
         ) / sizeof (struct storage_element_desc);

   storage_desc = (struct storage_element_desc *)&(storage_info[8]);
   for (i = 0; i < elements; i++,storage_desc++)
      {
         current_element = MAKE_WORD(    storage_desc->elem_address_msb,
                     storage_desc->elem_address_lsb);

         if (ac_device_ptr->storage_element_offset > current_element)
            ac_device_ptr->storage_element_offset = current_element;
      }

}

/*
 * This procedure gets the mailslot element address.
 * -paulc 10/26/1989 (modified)
 */
int
get_mailslot_address (ac_device_ptr,status_info)
   struct ac_device *ac_device_ptr;
   unsigned char *status_info;
{
   unsigned char   *in_out_info;
   struct in_out_element_desc {
      unsigned char   elem_address_msb,
            elem_address_lsb,
            elem_status,
            reservd;
      } *in_out_desc;
   int     elements,current_element,i;

   in_out_info = find_status_page(status_info,INPUT_OUTPUT_PC);

   ac_device_ptr->mailslot_address = 0x10000;      /* bigger than two bytes */

   elements = (    (in_out_info[5] << 16) |
         (in_out_info[6] << 8)  |
         in_out_info[7]
         ) / sizeof (struct in_out_element_desc);

   in_out_desc = (struct in_out_element_desc *)&(in_out_info[8]);
   for (i = 0; i < elements; i++,in_out_desc++)
      {
         current_element = MAKE_WORD(    in_out_desc->elem_address_msb,
                     in_out_desc->elem_address_lsb);

         if (ac_device_ptr->mailslot_address > current_element)
            ac_device_ptr->mailslot_address = current_element;
      }
}

#ifndef _WSIO

/*
 * Read the major numbers and the manager index information 
 * about the drives in the autochanger.
 */

int
ac_read_drives(ac_device_ptr)
   struct ac_device *ac_device_ptr;
{
   int i, error = 0;
   devnum_parms_type devnum;
   
   for (i=0;i<ac_device_ptr->number_drives;i++) {
      devnum.element = ac_device_ptr->ac_drive_table[i].element_address;
      error = autox0_ioctl(ac_device_ptr->device,AUTOX0_READ_DRIVE,&devnum);
      if (error)
         break;
      /* The major numbers should be the same for each drive */
      drive_maj_blk = devnum.b_major;
      drive_maj_char = devnum.c_major;

      ac_device_ptr->ac_drive_table[i].logical_unit = devnum.mgr_index;
      
   }
   return(error);
}
#endif



/*
 *
 * -paulc 10/26/1989 (modified)
 */
int
set_drive_info(ac_device_ptr,drive_info)
   struct ac_device *ac_device_ptr;
   unsigned char *drive_info;
{
   int flipped, el_desc_len, j;
   unsigned char *drive_entry;

   /* get element_descriptor length for use as offset */
   el_desc_len =
      MAKE_WORD(drive_info[2],drive_info[3]);

   /* number of drives is the number of elements */
   /* which is the number of data bytes / length of desc -paulc */
   ac_device_ptr->number_drives = (
         (drive_info[5] << 16) |
         (drive_info[6] << 8)  |
         drive_info[7]
         ) / el_desc_len;

   /* loop thru the element descriptors to get the other info */
   for (j = 0, drive_entry = &(drive_info[8]);
      j < ac_device_ptr->number_drives;
      drive_entry+=el_desc_len, j++) {

      /* get element address */
      ac_device_ptr->ac_drive_table[j].element_address =
         MAKE_WORD(drive_entry[0],drive_entry[1]);
            
      /* make sure that the scsi address is good */
      if (!(drive_entry[6] & ID_VAL_MSK)) {
         /* if any address is not set error */
         return (ENXIO);
      }

      /* make sure address is on the same bus */
      if (drive_entry[6] & NOT_THIS_BUS_MSK) {
         return (ENXIO);
      }

      /* get scsi address of element */
      ac_device_ptr->ac_drive_table[j].scsi_address =
         drive_entry[7];

      /* get source if valid */
      if (drive_entry[2] & FULL_MSK) {
         if (drive_entry[9] & SOURCE_VAL_MSK) {
            flipped = ((drive_entry[9] & INVERT_MSK) == INVERT_MSK);
            ac_device_ptr->ac_drive_table[j].surface =
               ELTOSUR(MAKE_WORD(drive_entry[10],drive_entry[11]),
                  ac_device_ptr->storage_element_offset,
                  flipped);
         } else {
            /* this should never happen */
            msg_printf2("No source for media in drive (SCSI id %d) in autochanger (SCSI id %d)\n",
               ac_device_ptr->ac_drive_table[j].scsi_address,
               m_busaddr(ac_device_ptr->device));
            return (ENXIO);
         }
      }
   }

#ifndef _WSIO
   return(ac_read_drives(ac_device_ptr));
#else
   return(0);
#endif
}

/*
 * This procedure gets the drive information for each drive. (scsi_address,
 * element_address, surface and num_drives)
 * -paulc 10/26/1989 (modified)
 */
int
get_drive_info (ac_device_ptr,status_info)
   struct ac_device *ac_device_ptr;
   unsigned char *status_info;
{
   unsigned char   *drive_info;

   drive_info = find_status_page(status_info,DATA_TRANSFER_PC);

   return (set_drive_info(ac_device_ptr,drive_info));
}

/*
 * This procedure creates a new surface request and initializes it. 
 */
void
init_sreq (sreqpp)
   struct surface_request **sreqpp;
{
   struct surface_request *sreqp;
   if ((sreqp = (struct surface_request *)kmem_alloc 
              ((u_int)sizeof (struct surface_request))) == NULL) {
      panic ("Error with kmem_alloc in init_req");
   } 
   *sreqpp = sreqp;
   (*sreqpp)->forw = *sreqpp;
   (*sreqpp)->back = *sreqpp;
   (*sreqpp)->surface = 0;
   (*sreqpp)->r_flags = 0;
   (*sreqpp)->pid = 0;
   (*sreqpp)->bp = NULL;
   (*sreqpp)->ac_device_ptr = NULL;
   (*sreqpp)->drive_ptr = NULL;
   (*sreqpp)->op = NULL;
   (*sreqpp)->op_parmp = NULL;
   (*sreqpp)->spinup_polls = 0;
}



/*
 * This procedure looks to see if there is a request in the queue for a surface
 * the same as the current surface.  It returns 1 if true, 0 if false.
 */
int
same_sur_in_q (current_surface,ac_device_ptr)
   int current_surface;
   struct ac_device *ac_device_ptr;
{
   struct surface_request *sreqp;

   FOR_EACH_SWAP_REQUEST
      if (sreqp->surface == current_surface) {
         RETURN_FROM_FOR(1);
      }
   END_FOR
   return (0);
}

/*
 * returns the appropriate surface depending on the state of the drive
 */
int
get_valid_surface (drive_ptr)
   struct ac_drive *drive_ptr;
{
   switch (drive_ptr->drive_state) {
   case DAEMON_CALL: 
   case FLUSH:
   case MOVE_OUT:
   case MOVE_IN:
   case SPINUP:
   case ALLOCATED:
         return(drive_ptr->surface_inproc);
   case DO_OP:
   case WAIT_TIME:
   case NOT_BUSY:
         return(drive_ptr->surface);
   default:
         panic("default case in get_valid_surface");
   }
   /*NOTREACHED*/
}


/*
 * This procedure returns the queue statistics.
 */
int
elements_in_q (ac_device_ptr)
   struct ac_device *ac_device_ptr;
{
   int i;
   char element_request_table[MAX_SUR/2];
   struct surface_request *sreqp;
   int element_count=0;

   /* zero the array */
   for (i=0; i<MAX_SUR/2; i++) {
      element_request_table[i] = 0;
   }

   /* find the elements used in the queue */
   FOR_EACH_SWAP_REQUEST
      /* by using an offset of 0              */
      /* SURTOEL() will put numbers in range 0 thru (MAX_SUR/2)-1 */
      element_request_table[SURTOEL(sreqp->surface,0)] = 1;
   END_FOR

   /* count the elements */
   for(i=0; i<MAX_SUR/2; i++) {
      if (element_request_table[i] == 1) {
         element_count++;
      }
   }
   return(element_count);
}

/*
 * This procedure looks to see if the flip side of a surface is somewhere in
 * the queue.
 */
flip_side_in_q (surface,ac_device_ptr)
   int surface;
   struct ac_device *ac_device_ptr;
{
   int req_el, el;
   struct surface_request *sreqp;

   el = SURTOEL(surface,ac_device_ptr->storage_element_offset);
   FOR_EACH_SWAP_REQUEST
      req_el = SURTOEL(sreqp->surface,ac_device_ptr->storage_element_offset);
      if ((surface != sreqp->surface) && (el == req_el)) {    
         RETURN_FROM_FOR(1);
      }
   END_FOR
   return(0);
}

/*
 * This procedure looks to see if there are more requests for surfaces 
 * than there are resources (drives).
 * What this boils down to is, are there any surfaces in the requeust queue that
 * are not currently in a drive or on the way. 
 * It returns 1 if true, 0 if false.
 */
int
requests_exceed_resources (ac_device_ptr)
   struct ac_device *ac_device_ptr;
{
   struct surface_request *sreqp;
   int i,surface_inserted;
   int empty_drive = 0;
   int found_flip_side = 0;
   int surface;

   for (i=0; i<ac_device_ptr->number_drives; i++) {
      surface = get_valid_surface(&(ac_device_ptr->ac_drive_table[i]));
      if(surface == EMPTY) {
         empty_drive = 1;
      } else {
         found_flip_side = flip_side_in_q(surface,ac_device_ptr);
      }
   }

   if (empty_drive) {
      if (found_flip_side) 
         /* can't service a flip side in an empty drive */
         return(1);
      else
         return(0);
   } else {
      FOR_EACH_SWAP_REQUEST
         surface_inserted = 0;
         for (i=0; i<ac_device_ptr->number_drives; i++) {
            if (sreqp->surface == 
               get_valid_surface(&(ac_device_ptr->ac_drive_table[i])))
               surface_inserted = 1;
         }
         if (!surface_inserted) {
            /* found surface that is NOT inserted */
            RETURN_FROM_FOR(1);
         }
      END_FOR
      /* didn't find any requests for surfaces not inserted */
      return (0);
   }
}

/*
 * This procedure checks to see if the current element is inserted in any of
 * the drives.  It returns 1 if true (passing back the drive_ptr),
 * 0 if false.
 */
int 
current_element_inserted (ac_device_ptr, current_surface, drive_ptr)
   struct ac_device *ac_device_ptr;
   int current_surface;
   struct ac_drive **drive_ptr;
{
   int i;
   struct ac_drive *temp_drive_ptr;

   /* loop thru all the drives */
   for (i = 0; i < ac_device_ptr->number_drives; i++) {
      temp_drive_ptr = &(ac_device_ptr->ac_drive_table[i]);
      if (SURTOEL(current_surface,
             ac_device_ptr->storage_element_offset) == 
          SURTOEL(get_valid_surface(temp_drive_ptr),
             ac_device_ptr->storage_element_offset)) {
         /*pass back drive with current element*/
         *drive_ptr = temp_drive_ptr;
         return (1);   /* current element is inserted */
      }
   }
   return (0);  /* current element not inserted */
}



/*
 * This procedure checks to see if the current element is inserted in any of
 * the drives.  It returns 1 if true (passing back the drive_ptr),
 * 0 if false.
 */
int 
current_element_really_inserted (ac_device_ptr, current_surface, drive_ptr)
   struct ac_device *ac_device_ptr;
   int current_surface;
   struct ac_drive **drive_ptr;
{
   int i;
   struct ac_drive *temp_drive_ptr;

   /* loop thru all the drives */
   for (i = 0; i < ac_device_ptr->number_drives; i++) {
      temp_drive_ptr = &(ac_device_ptr->ac_drive_table[i]);
      if (SURTOEL(current_surface,
             ac_device_ptr->storage_element_offset) == 
          SURTOEL(temp_drive_ptr->surface,
             ac_device_ptr->storage_element_offset)) {
         /*pass back drive with current element*/
         *drive_ptr = temp_drive_ptr;
         return (1);   /* current element is inserted */
      }
   }
   return (0);  /* current element not inserted */
}


/*
 * This procedure checks to see if the current surface is inserted in any of
 * the drives.  It returns 1 if true (passing back the drive_ptr),
 * 0 if false.
 */
int 
current_sur_inserted (ac_device_ptr, current_surface, drive_ptr)
   struct ac_device *ac_device_ptr;
   int current_surface;
   struct ac_drive **drive_ptr;
{
   int i;
   struct ac_drive *temp_drive_ptr;

   /* loop thru all the drives */
   for (i = 0; i < ac_device_ptr->number_drives; i++) {
      temp_drive_ptr = &(ac_device_ptr->ac_drive_table[i]);
      if ((temp_drive_ptr->drive_state == DO_OP) ||
          (temp_drive_ptr->drive_state == WAIT_TIME) ||
          (temp_drive_ptr->drive_state == FLUSH) ||
          (temp_drive_ptr->drive_state == ALLOCATED) ||
          (temp_drive_ptr->drive_state == DAEMON_CALL) ||
          (temp_drive_ptr->drive_state == NOT_BUSY)) {
         if (current_surface == temp_drive_ptr->surface) {
            /*pass back drive with current surface*/
            *drive_ptr = temp_drive_ptr;
            return (1);   /* current surface is inserted */
         }
      }
   }
   return (0);  /* current surface not inserted */
}

/*
 * This procedure passes back the "proper" drive_ptr depending on the 
 * surface that the request is for. 
 */
void
get_drive (ac_device_ptr, current_surface, drive_ptr)
   struct ac_device *ac_device_ptr;
   int current_surface;
   struct ac_drive **drive_ptr;
{
   int i, found_nonbusy = 0,found_surface=0;
   struct timeval atv;
   struct ac_drive *temp_drive_ptr,*empty_drive_ptr = NULL;
   long oldest_time;

   GET_TIME(atv);
   oldest_time = atv.tv_sec;
   oldest_time += 1000;    /* make time be in the future by a lot */

   /* if the surface is already inserted, use that drive */
   if (current_element_inserted (ac_device_ptr, current_surface, drive_ptr)) { 
      return;
   }

   /* loop thru all the drives */
   for (i = 0; i < ac_device_ptr->number_drives; i++) {
      temp_drive_ptr = &(ac_device_ptr->ac_drive_table[i]);
      if (temp_drive_ptr->drive_state == NOT_BUSY)  {
         found_nonbusy = 1;
         /* return the first empty drive */
         if (temp_drive_ptr->surface == EMPTY) {
            *drive_ptr = temp_drive_ptr;
            if (temp_drive_ptr->close_request) {
               if (temp_drive_ptr->close_request->move_request.source ==
                  SURTOEL (current_surface,
                     ac_device_ptr->storage_element_offset)) {
                  /* current surface might be delayed closed*/
                  /* surface is empty but cart still in drive */
                  found_surface = 1;
                  break;
               }
            }
            else
               /* return an emptry drive rather than one */
               /* that is in close_delay if it is not for */
               /* that surface. */
               empty_drive_ptr = temp_drive_ptr;
         }
         /* pass back oldest, non-busy drive */
         if(oldest_time > temp_drive_ptr->hog_time_start) {
            *drive_ptr = temp_drive_ptr;
            oldest_time = temp_drive_ptr->hog_time_start;
         }
      }
   }

   if(found_nonbusy) {
      if (found_surface) {
         /* msg_printf1("found_surface in get drive drive = %x\n",*drive_ptr); */
         return;
      }
      else {
         if (empty_drive_ptr) {
            *drive_ptr = empty_drive_ptr;
            /* msg_printf1("Returning empty drive, drive = %x\n",*drive_ptr); */
            return;
         }
         /* msg_printf1("Returning oldest drive, drive = %x\n",*drive_ptr); */
         return;
         }
   } else {
      panic("all drives busy in get_drive");
   }
   /*NOTREACHED*/
}

/*
 * Return a pointer to a drive that's hog time has expired and in the
 * state WAIT_TIME.
 */
struct ac_drive *
get_expired_drive(ac_device_ptr)
   struct ac_device *ac_device_ptr;
{
   struct ac_drive *drive_ptr = NULL;
   int i;
   struct timeval atv;
   struct ac_drive *temp_drive_ptr;
   long oldest_time;

   GET_TIME(atv);
   oldest_time = atv.tv_sec;
   oldest_time += 1000;    /* make time be in the future by a lot */

   /* loop thru all the drives */
   for (i = 0; i < ac_device_ptr->number_drives; i++) {
      temp_drive_ptr = &(ac_device_ptr->ac_drive_table[i]);
      if ((!hog_time_ok(temp_drive_ptr)) &&
         (temp_drive_ptr->drive_state == WAIT_TIME)) {
         /* pass back oldest, expired, waiting drive */
         if(oldest_time > temp_drive_ptr->hog_time_start) {
            drive_ptr = temp_drive_ptr;
            oldest_time = temp_drive_ptr->hog_time_start;
         }
      }
   }
   return(drive_ptr);
}

/*
 * This procedure checks to see if the hog time for a particular drive is OK. 
 * It returns 1 if the got_time has not expired and 0 if it has expired.
 */
int 
hog_time_ok (drive_ptr)
   struct ac_drive *drive_ptr;
{
   struct timeval atv;

   int surface;
   surface = get_valid_surface(drive_ptr);

   GET_TIME(atv);
   if ((atv.tv_sec - drive_ptr->hog_time_start) >
            (long)(drive_ptr->ac_device_ptr->ac_hog_time[surface])) {
      return (0); /* NOT ok */
   } else {
      return (1); /* ok */
   }
}

/*
 * This procedure checks to see if the drives are busy.  This can be true 
 * if: 1) all drives are busy (all_busy) or
 *     2) the drive with the current surface is busy (cur_sur_busy).
 * It returns 1 if and 0 if false.
 */
int
drives_busy (ac_device_ptr, current_surface)
   struct ac_device *ac_device_ptr;
   int current_surface;
{
   int i, all_busy, cur_sur_busy;
   struct ac_drive *drive_ptr;

   /* initialize booleans */
   all_busy = 1;     /* true */
   cur_sur_busy = 0; /* false */

   /* loop thru drives */
   for (i = 0; i < ac_device_ptr->number_drives; i++) {
      drive_ptr = &(ac_device_ptr->ac_drive_table[i]);
      if (drive_ptr->drive_state == NOT_BUSY) {
         all_busy = 0;
      }
      if ((SURTOEL (get_valid_surface(drive_ptr),
               ac_device_ptr->storage_element_offset) ==
          (SURTOEL (current_surface,
               ac_device_ptr->storage_element_offset))) &&
          (drive_ptr->drive_state != NOT_BUSY)) {
         cur_sur_busy = 1;
      }
   }
   return (all_busy || cur_sur_busy);
}

/*
 * This procedure gets the next request, skipping requests for surfaces that
 * are already in the drives.
 */
struct surface_request *
get_next_request (drive_ptr,ac_device_ptr)
   struct ac_drive *drive_ptr;
   struct ac_device *ac_device_ptr;
{
   int i, return_it, req_el, drive_el;
   struct surface_request *sreqp;
   
   /* choose the next request to run */
   FOR_EACH_SWAP_REQUEST
      /* loop through request queue looking for a request */
      /* that is NOT busy and NOT in one of the drives */
      if (!(sreqp->r_flags & R_BUSY)) {
         return_it = 1;
         for (i = 0; i < ac_device_ptr->number_drives; i++) {
            /* if the element for this request is in
             * a different drive, don't return it.
             */
            if (drive_ptr != &(ac_device_ptr->ac_drive_table[i])) {
               /* see if other drive(s) will
                * execute this request
                */
               req_el = SURTOEL(sreqp->surface, ac_device_ptr->storage_element_offset); 
               drive_el = SURTOEL(get_valid_surface(&ac_device_ptr->ac_drive_table[i]), 
                  ac_device_ptr->storage_element_offset);

               if (req_el == drive_el) {
                  return_it = 0;  
               }
            }
         }
         if (return_it) {
            /* mark that request busy */
            sreqp->r_flags |= R_BUSY;
            RETURN_FROM_FOR(sreqp);
         }
      }
   END_FOR
   return(NULL);
}

/*
 * This procedure sends an xport request to the xport daemon.
 * The pointer to the xport_request is returned.
 */

struct xport_request *
send_xport_request(ac_device_ptr,sreqp,xportp)
   struct ac_device *ac_device_ptr;
   struct surface_request *sreqp;
   struct xport_request *xportp;
{
   struct xport_request *xport_reqp;

   ac_device_ptr->transport_busy = 1;
   if (xportp) {
      xport_reqp = xportp;
      sreqp->drive_ptr->interrupt_request = NULL;
      }
   else {
      xport_reqp = init_xport_request();
   }
   if (AC_DEBUG & XPORT_OP)
      msg_printf1("SXR 0x%x ",xport_reqp);
   /* Put in the move parameters */
   xport_reqp->move_request.transport = ac_device_ptr->transport_address;
   xport_reqp->move_request.source = SURTOEL(sreqp->surface,
                  ac_device_ptr->storage_element_offset);
   xport_reqp->move_request.destination = sreqp->drive_ptr->element_address;
   xport_reqp->move_request.invert = FLIP(sreqp->surface);
   xport_reqp->pid = sreqp->pid;
   xport_reqp->sreqp = sreqp; /* only needed for async requests */
   if (AC_DEBUG & XPORT_OP)
      msg_printf1("SXRS 0x%x\n",sreqp);
   if (sreqp->r_flags & R_ASYNC) 
      xport_reqp->x_flags |=  X_ASYNC;
   sreqp->drive_ptr->drive_state = DAEMON_CALL;
   /* keep other requests out while deamon starts up */
   if (AC_DEBUG & QUEUES) {
      msg_printf0("XPORT");
      print_xport_queue(ac_device_ptr->xport_queue_head);
   }
   ADD_TO_LIST(ac_device_ptr->xport_queue_head,xport_reqp);
   if (AC_DEBUG & QUEUES) 
      print_xport_queue(ac_device_ptr->xport_queue_head);

   wakeup(&(ac_device_ptr->xport_queue_head));

   return(xport_reqp);

}


/*
 * This procedure starts a request.
 */
void
startup_request(sreqp,xportp)
   struct surface_request *sreqp;
   struct xport_request *xportp;
{
   struct xport_request *xport_reqp;
   struct ac_device *ac_device_ptr; 
   extern void allocate_drive();
   extern void wait_time_done();

   if (sreqp != NULL)  {
      ac_device_ptr = sreqp->ac_device_ptr;
      /* so that no other process can allocate it before the freshly */
      /* awoken process begins execution -- Paul C.*/

      allocate_drive(sreqp);
      /* Clear any timeouts for this surface */
      untimeout(wait_time_done, (caddr_t)(sreqp->drive_ptr));
      if (sreqp->r_flags & R_ASYNC) { /* asynchronous request */
         /* Must start the xport daemon, no process to wakeup */
         if (AC_DEBUG & OPERATION)
            msg_printf1("Startup request: async surface = %d\n",sreqp->surface); 
         xport_reqp = send_xport_request(ac_device_ptr,sreqp,xportp);
      }
      else {
         wakeup(sreqp);
      }
   }
}

/*
 * This procedure executes if the process wakes up after ac_wait_time.
 */

void
wait_time_done (drive_ptr)
   struct ac_drive *drive_ptr;
{
   struct surface_request *sreqp;
   int pri;

   /* critical section to protect from wait_time_done */
   pri = spl6();

   drive_ptr->drive_state = NOT_BUSY;

   if (AC_DEBUG & WAIT_CASE) {
      msg_printf0("   wait_time switch\n");
      msg_printf1("      ref_count=%d\n",drive_ptr->ref_count);
      msg_printf1("      drive_ptr=%x\n",drive_ptr);
   }
   if (drive_ptr->ref_count == 0) {/* no other processes using the drive */
      sreqp = get_next_request (drive_ptr,drive_ptr->ac_device_ptr);
      startup_request(sreqp,drive_ptr->interrupt_request);
   }
   splx(pri);
}

/*
 * This procedure waits for ac_wait_time and then executes the procedure
 * wait_time_done.
 */
int
wait_for_wait_time (drive_ptr)
   struct ac_drive *drive_ptr;
{
   extern void wait_time_done();
   int surface;

   surface = get_valid_surface(drive_ptr);

   drive_ptr->drive_state = WAIT_TIME;
   if (AC_DEBUG & WAIT_CASE) 
      msg_printf1("wt %x\n",drive_ptr);
   timeout (wait_time_done, (caddr_t)drive_ptr,
         HZ * drive_ptr->ac_device_ptr->ac_wait_time[surface]);
}


/*
 * Determine if media in drive needs to be flipped over to service request.
 */
int
needs_flip (surface,drive_ptr)
   int surface;
   struct ac_drive *drive_ptr;
{
   struct ac_device *ac_device_ptr = drive_ptr->ac_device_ptr;
   int inserted_surface = drive_ptr->surface;
   int inserted_element = SURTOEL(inserted_surface,
               ac_device_ptr->storage_element_offset);
   int requested_element = SURTOEL(surface,
               ac_device_ptr->storage_element_offset);

   /* Test to see if we want to use the flip side of this media */
   /* (see if element numbers match and surfaces don't match)   */

   if ((inserted_element == requested_element) &&
       (inserted_surface != surface)) {
      return(1);      /* needs flip */
   } else {
      return(0);      /* doesn't need flip */
   }
}

/*
 *  This routines checks to see if the move succeeded
 *  when a power failure error was returned from a 
 *  move command.  Just checks to see if source is full.
 *  Return of 0 means no retry needed, return of non-zero
 *  means a retry is required because the destination is
 *  empty.
 */

int
need_to_retry_move(ac_device_ptr, move_parms_ptr)
   struct ac_device *ac_device_ptr;
   struct move_medium_parms *move_parms_ptr;
{
   int starting_element_address, el_desc_offset, return_value;
   int element, el_desc_len, error = 0, page_code, j;
   int drive_count, drive_el, requested_sur, surface_now;
   struct buf *el_bp;
   unsigned char *status_info;
   unsigned char *storage_info;
   unsigned char *drive_info;
   unsigned char *drive_entry;

   element = move_parms_ptr->destination;

/* Check to see if this destination is full */

/*
 * get element status atomically, to avoid syncronization problems
 * which can occur when the autochanger is in motion
 * -paulc 10/26/1989
 */
   el_bp      = geteblk(EL_STATUS_SZ);
   el_bp->b_dev    = ac_device_ptr->device;
   page_code       = ALL_PC;
   ac_read_element_status(ac_device_ptr,el_bp,page_code);
   if (error = geterror(el_bp)) {
      brelse(el_bp);
      return(error);
   }

   status_info = (unsigned char *)el_bp->b_un.b_addr;

   storage_info = find_status_page(status_info,STORAGE_PC);

   /* get starting element_address */
   starting_element_address = ac_device_ptr->storage_element_offset;

   /* get element_descriptor length for use as offset */
   el_desc_len = MAKE_WORD(storage_info[2],storage_info[3]);

   /* get the offset in the data for the particular element */
   el_desc_offset = PC_HDR_LEN 
          + (element - starting_element_address) * el_desc_len;

   /* check if this element is full */
   if (element >= starting_element_address)  {
      if (storage_info[el_desc_offset + 2] & FULL_MSK) 
         return_value = 0;
      else
         return_value = 1;
   } else {  /* dest is a drive */
      return_value = ENXIO;

      drive_info = find_status_page(status_info,DATA_TRANSFER_PC);
   
      /* get element_descriptor length for use as offset */
      el_desc_len = MAKE_WORD(drive_info[2],drive_info[3]);
   
      /* number of drives is the number of elements */
      /* which is the number of data bytes / length of desc -paulc */
      drive_count = (
            (drive_info[5] << 16) |
            (drive_info[6] << 8)  |
            drive_info[7]
            ) / el_desc_len;
   
      /* loop thru the element descriptors to get the other info */
      for (j = 0, drive_entry = &(drive_info[8]);
         j < drive_count;
         drive_entry+=el_desc_len, j++) {

         drive_el = (MAKE_WORD(drive_entry[0],drive_entry[1])); 

         /* compare source element address, find the right dest drive */
         if ( element == drive_el ) {

            requested_sur = ac_device_ptr->ac_drive_table[j].surface_inproc;
            surface_now = ELTOSUR((MAKE_WORD(drive_entry[10],drive_entry[11])),
                                  starting_element_address,
                                  ((drive_entry[9] & INVERT_MSK) == INVERT_MSK));

            /* if the drive is full and the surface in it now is
             * the requested surface, no retry is needed
             */
            if ((drive_entry[2] & FULL_MSK) && (requested_sur == surface_now))
               return_value = 0;
            else
               return_value = 1;
            break;
         }
      }
   
   }

#ifdef KERNEL_DEBUG_ONLY
   if (force_error.surface_in_ac) return_value = ENXIO;
#endif KERNEL_DEBUG_ONLY
   brelse(el_bp);
   return (return_value);
}


/*
 * This module takes care of issuing the move_medium command to the driver
 * given the source, destination and flip.  An error is returned.
 */
int
move_medium (ac_device_ptr, move_parms_ptr)
   struct ac_device *ac_device_ptr;
   struct move_medium_parms *move_parms_ptr;
{
   int error = 0;
   int pri;

   /* load up move_medium structure */
   move_parms_ptr->transport = ac_device_ptr->transport_address;
   if (AC_DEBUG & XPORT_OP) {
      msg_printf1("\tTransport   = %d\n",move_parms_ptr->transport);
      msg_printf1("\tSource      = %d\n",move_parms_ptr->source   );
      msg_printf1("\tDestination = %d\n",move_parms_ptr->destination);
      msg_printf1("\tInvert      = %d\n",move_parms_ptr->invert     );
   }

   /* call driver to switch media */
#ifdef KERNEL_DEBUG_ONLY
   if (force_error.move_to_drive) error = EIO;
   else
#endif KERNEL_DEBUG_ONLY
   error = autox0_ioctl(ac_device_ptr->device,AUTOX0_MOVE_MEDIUM, move_parms_ptr);
   while (error == ECONNRESET)  {
      rereserve(ac_device_ptr);
      if (AC_DEBUG & XPORT_OP) {
         msg_printf0("Power fail error detected, ");
      }
      if (need_to_retry_move(ac_device_ptr, move_parms_ptr)) {
         msg_printf0("retry move\n"); 
         error = autox0_ioctl(ac_device_ptr->device,AUTOX0_MOVE_MEDIUM, move_parms_ptr);
      } else {
         error = 0;
         msg_printf0("don't retry move\n"); 
      }
   }
   if (error) {
      msg_printf1("move medium failed, ERROR = %d\n",error);
   }
   return(error);
}

/*
 * Take media out of drive and return it to its home slot.
 * An error is returned.
 */
int
move_medium_home(ac_device_ptr,drive_ptr)
   struct ac_device *ac_device_ptr;
   struct ac_drive *drive_ptr;
{
   int inserted_surface = drive_ptr->surface;
   int error;

   drive_ptr->drive_state = MOVE_OUT;

   drive_ptr->move_parms_ptr->transport = ac_device_ptr->transport_address;
   drive_ptr->move_parms_ptr->source = drive_ptr->element_address;
   drive_ptr->move_parms_ptr->destination = SURTOEL(inserted_surface,ac_device_ptr->storage_element_offset),
   drive_ptr->move_parms_ptr->invert = FLIP (inserted_surface);

#ifdef KERNEL_DEBUG_ONLY
   if (force_error.move_home) error = EIO;
   else
#endif KERNEL_DEBUG_ONLY
   error = move_medium (ac_device_ptr,drive_ptr->move_parms_ptr);
   
   return(error);
}

/*
 * This procedure takes care of spinning up the drive.
 */
int
do_spinup(drive_ptr,surface)
   struct ac_drive *drive_ptr;
   int surface;
{
   int error = 0;
   struct timeval atv;

   drive_ptr->drive_state = SPINUP;

   /* mark drive as empty while waiting for spinup */
   drive_ptr->surface = EMPTY;
   error = wait_for_spinup(drive_ptr);

   /* put the proper surface in media data structure */
   drive_ptr->surface = surface;

   GET_TIME(atv);
   drive_ptr->hog_time_start = atv.tv_sec;
   return(error);
}

/*
 * This procedure polls the drive until it has spun up.  It returns an
 * error if after a time the drive is still not responding.
 */

int
wait_for_spinup(drive_ptr)
   struct ac_drive *drive_ptr;
{
   int i,j;
   int error = 0;
   int priority;
   struct ac_device *ac_device_ptr = drive_ptr->ac_device_ptr;

   if (AC_DEBUG & SPINUP_OP)
      msg_printf0("starting spinup\n"); 

   /* must wait for drive to spin up */
   /* wait MAX_WAIT_INTERVALS half-second intervals for it to spin up */
   for (i=0;i<MAX_WAIT_INTERVALS;i++) {
      priority = spl5(); 
      if (i == 0)
         timeout (wakeup, (caddr_t)drive_ptr, HZ*INITIAL_WAIT_INTERVAL/1000);
      else
         timeout (wakeup, (caddr_t)drive_ptr, HZ*WAIT_INTERVAL/1000);
#ifdef KERNEL_DEBUG_ONLY
      if (force_error.wait_spinup_sleep) {
         error = -1;
         psignal(u.u_procp,SIGINT);
      } else
#endif KERNEL_DEBUG_ONLY
      error = sleep(drive_ptr,PRIBIO);
      splx(priority); 
      if (error) {
         return(EINTR);
      }
      untimeout (wakeup, (caddr_t)drive_ptr);
#ifndef _WSIO
      error = (*cdevsw[drive_maj_char].d_open)(makedev(drive_maj_char,
            makeminor(0,drive_ptr->logical_unit,0,2)));
#else
      if (AC_DEBUG & SPINUP_OP)
         msg_printf0("starting open\n"); 
      error = (*cdevsw[drive_maj_char].d_open)(makedev(drive_maj_char,
               ac_minor(ac_card(ac_device_ptr->device),drive_ptr->scsi_address,0,0)),0);

      if (AC_DEBUG & SPINUP_OP)
         msg_printf0("ended open\n"); 
#endif
      if (error == 0) {
         drive_ptr->opened = 1;
         if (AC_DEBUG & SPINUP_OP)
            msg_printf2("Opened drive %x, opened = %d\n",drive_ptr->scsi_address,drive_ptr->opened);
         break;
      }
   }
#ifdef KERNEL_DEBUG_ONLY
   if (force_error.wait_spinup) error = EIO;
   else
#endif KERNEL_DEBUG_ONLY
   if (i>=MAX_WAIT_INTERVALS) {
      /* return error that driver did not spin up */
      for(j=0;j<NUM_DRIVE;j++) {
         if (drive_ptr == &(ac_device_ptr->ac_drive_table[j]))
            break;
      }
      msg_printf1("DRIVE %d not spun up!!!\n",j+1);
      error = EIO;
   }
   if (AC_DEBUG & SPIN_TIME) {
      msg_printf1("spin-up took %d intervals.\n",i);
   }
   return(error);
}



/*
 * This function returns a pointer to a ac_drive struct if the element
 * address matches a drive element address.
 * Returns NULL if no match if found.
 */
struct ac_drive *
match_element(ac_device_ptr,element_addr)
   struct ac_device *ac_device_ptr;
   int element_addr;
{
   int i;

   for (i=0; i<ac_device_ptr->number_drives; i++) {
      if (ac_device_ptr->ac_drive_table[i].element_address ==
                        element_addr) {
         return(&(ac_device_ptr->ac_drive_table[i]));
      }
   }
   /* no match found, return error */
   return ((struct ac_drive *)0);
}

/*
 * This routine clears all close delays and makes sure
 * no more occur.  This is to allow rebooting the system
 * and have all the buffers flushed to the drives correctly
 * before the system shuts down.
 * 
 */

void
clear_all_close_delays()

{

   struct ac_device *ac_device_ptr;
   struct xport_request *xport_reqp;
   int pri;

   /* loop through all the autochangers flushing the closes
      and setting ac_close_time to zero to avoid any future
      delayed closes 
    */
   ac_device_ptr = ac_device_head;
   if (ac_device_ptr != NULL) {
      do {
         /* set the close time to zero */
         ac_device_ptr->ac_close_time = 0;

        /* release all the buffers used by the daemons */
    if (ac_device_ptr->xport_bp) {
       brelse(ac_device_ptr->xport_bp);
       ac_device_ptr->xport_bp = 0;
       }
    if (ac_device_ptr->spinup_bp) {
       brelse(ac_device_ptr->spinup_bp);
       ac_device_ptr->spinup_bp = 0;
    }

         /* flush all the delayed closes */
         /* Make sure the device is opened */ 
         ac_device_ptr->transport_busy = 1;
         xport_reqp = init_xport_request();
         /* no move parameters necessary */
         xport_reqp->pid = 0;
         xport_reqp->x_flags |=  X_FLUSH_CLOSE;
         /* keep other requests out while deamon starts up */
         if (AC_DEBUG & QUEUES) {
            msg_printf0("XPORT");
            print_xport_queue(ac_device_ptr->xport_queue_head);
         }
         ADD_TO_LIST(ac_device_ptr->xport_queue_head,xport_reqp);
         if (AC_DEBUG & QUEUES) 
            print_xport_queue(ac_device_ptr->xport_queue_head);

         pri = spl6();    
         wakeup(&(ac_device_ptr->xport_queue_head));
         sleep(xport_reqp,PRIBIO);
         (void) splx(pri);
         FREE_ENTRY(xport_reqp,xport_request);
   
         /* go to next autochanger */
         ac_device_ptr = ac_device_ptr->forw;
      } while (ac_device_ptr != ac_device_head);
   }
}
    


/*
 * Entry point to allow file systems to tell the ac driver
 * what to do to the surface to maintain file system integrity.
 * The dev is the device of the surface.
 * pmount is called when the surface is inserted into a drive.
 * punmount is called when the surface is removed from a drive.
 * 
 * To cancel these functions when the surface is unmounted by the
 * superuser, the file system should call this entry point
 * with mount_ptr and the two function pointers set to null.
 */

void
autoch_mount(dev,mount_ptr,pmount,punmount,reboot)
   dev_t dev;
   caddr_t mount_ptr;
   int (*pmount) ();
   int (*punmount) ();
   int reboot;

{
   /* determine which surface */
   int surface = ac_surface (dev);
   int section = ac_section (dev);
   struct ac_device *ac_device_ptr;

   /*  NOTE:  If reboot flag has been set, the other
              parameters are ignored.  */
   if (reboot) {
      clear_all_close_delays();
      return;
   }
   if ((ac_device_ptr = ac_opened(dev)) == NULL) {
      panic ("autoch_mount: unopened autochanger");
   }

   while (ac_device_ptr->pmount_lock & (IN_MOUNT | IN_PMOUNT | IN_PUNMOUNT)) {
      ac_device_ptr->pmount_lock |= WANT_MOUNT;
      sleep(&(ac_device_ptr->pmount_lock),PRIBIO);
   }
   ac_device_ptr->pmount_lock |= IN_MOUNT;


   /* Only set the entries if the structure exists.  This is to
      avoid a race condition with the boot_clean_fs() and the mount code.
      The mount code has created the mount data structures but has not
      yet sent the open to the device but the reboot code is trying to
      clear the mount entries based upon the mount data structures. */

   if (ac_device_ptr->elements[surface][section]) {
      ac_device_ptr->elements[surface][section]->mount_ptr = mount_ptr;
      ac_device_ptr->elements[surface][section]->pmount = pmount;
      ac_device_ptr->elements[surface][section]->punmount = punmount;
   }

   if (ac_device_ptr->pmount_lock & (WANT_MOUNT | WANT_PMOUNT | WANT_PUNMOUNT))
      wakeup((caddr_t)&(ac_device_ptr->pmount_lock));
   ac_device_ptr->pmount_lock &= ~(WANT_MOUNT | IN_MOUNT);

   if (AC_DEBUG & PMOUNT)
      msg_printf3("autoch_mount called, %x, %x, %x\n",mount_ptr,pmount,punmount); 
}


/*
 * This routine sets exclusive at the disk driver level
 */

int 
autoch_set_exclusive_mode(dev, v, drive_ptr)
   dev_t dev;
   caddr_t v;
   struct ac_drive *drive_ptr;
{
   int error;

   if (AC_DEBUG & MP_IOCTL)
      msg_printf("autoch_set_exclusive_mode\n");

   error = (*cdevsw[drive_maj_char].d_ioctl)(dev, DIOC_EXCLUSIVE, v, FWRITE);

   return (error);
}

/*
 * This routine sets verify writes at the disk driver level
 */

int 
autoch_set_wwv_mode(dev, v, drive_ptr)
   dev_t dev;
   caddr_t v;
   struct ac_drive *drive_ptr;
{
   int error;

   if (AC_DEBUG & MP_IOCTL)
      msg_printf("autoch_set_wwv_mode\n");

   error = (*cdevsw[drive_maj_char].d_ioctl)(dev, SIOC_VERIFY_WRITES, v, FWRITE);

   return (error);
}

/*
 * This routine sets write without erase at the disk driver level
 */

int 
autoch_set_woe_mode(dev, v, drive_ptr)
   dev_t dev;
   caddr_t v;
   struct ac_drive *drive_ptr;
{
   int error;
   int flag = 1;

   if (AC_DEBUG & MP_IOCTL)
      msg_printf("autoch_set_woe_mode\n");

   if ((error = (*cdevsw[drive_maj_char].d_ioctl)(dev, DIOC_EXCLUSIVE, &flag, FWRITE)) == 0) {
      error = (*cdevsw[drive_maj_char].d_ioctl)(dev, SIOC_WRITE_WOE, v, FWRITE);
   }

   return (error);
}

/*
 * This routine is called during pmount_section (when a surface gets
 * reloaded into a drive) and sets the flags at the disk level to 
 * the values before it was swapped out.  During pumount_section all
 * the flags are cleared by calling close at the disk level.  Thus
 * all the flags have to be turned back on.
 */

int
autoch_set_drive_mode(drive_ptr, section) 
   struct ac_drive *drive_ptr;
   int section;
{
   int error = 0;
   int flag = 1;
   dev_t drive_dev;

   if (AC_DEBUG & MP_IOCTL)
      msg_printf2("autoch_set_drive_mode section is %d drive is 0x%x\n", section, drive_ptr);
#ifndef _WSIO
      drive_dev = makedev(drive_maj_char, makeminor(0,drive_ptr->logical_unit,0,section));
#else
      drive_dev = makedev(drive_maj_blk, ac_minor(ac_card(drive_ptr->ac_device_ptr->device),
                          drive_ptr->scsi_address,0,0));
#endif

   if (drive_ptr->ac_device_ptr->phys_drv_state[drive_ptr->surface] & S_IMMED)
      error = autoch_sioc_set_ir(drive_dev, &flag, drive_ptr);

   if (section == ENTIRE_SURFACE) {

      if (drive_ptr->ac_device_ptr->elements[drive_ptr->surface][section]->exclusive_lock == 1) 
         error = autoch_set_exclusive_mode(drive_dev, &flag, drive_ptr);

      if (drive_ptr->ac_device_ptr->elements[drive_ptr->surface][section]->section_flags & S_WWV) 
         error = autoch_set_wwv_mode(drive_dev, &flag, drive_ptr);

      if (drive_ptr->ac_device_ptr->elements[drive_ptr->surface][section]->section_flags & S_WOE) 
         error = autoch_set_woe_mode(drive_dev, &flag, drive_ptr);

   } 
   else error = EINVAL;
   return (error);
}

void
autoch_clear_drive_state(dev, drive_ptr)
   dev_t dev;
   struct ac_drive *drive_ptr;
{
   int error;
   int flag = 0;  /* turn of the set flags if any */

   /* these errors are being ignored because this routine */
   /* is only being called on the final close of the device */
   /* and error recovery is not needed */

   error = autoch_set_woe_mode(dev, &flag, drive_ptr);
   error = autoch_set_wwv_mode(dev, &flag, drive_ptr);
   error = autoch_set_exclusive_mode(dev, &flag, drive_ptr);

} 

void
autoch_clear_phys_drive(dev, drive_ptr, drvr_state)
   dev_t dev;
   struct ac_drive *drive_ptr;
   int drvr_state;
{
   int error;
   int flag = 0;  /* turn off the set flags if any */

   /* always want to clear the flags on first open, so drives are in a known state */

   if (((drive_ptr->ac_device_ptr->phys_drv_state[drive_ptr->surface]) & S_IMMED) || (drvr_state & INIT))
      error = autoch_sioc_set_ir(dev, &flag, drive_ptr);

}

/*
 * Performs the physical mount of the surface.  If the
 * file system has set the pmount functions, call them 
 * as well.  This routine is called for every surface
 * after it is inserted into a drive.
 * The autochanger must have a cartridge in the specified drive.
 * The drive is open when this routine is called.
**
** The call for the surf_dev = makedev(...) is set up as so:
** The surface_dev is set up a 0x012xxx with each field set up as
** as 4-bit field like  0x4 4 4 4 4 4 ***
**                        |   | | | 
**                        |   | | Surface Number for 288 surfaces
**                        |   | X X X S -- reserved plus the MSB of surface number total of 9 bits --
**                        |   section number
**                        lu of autochanger for autox0
*/
void
pmount_section(drive_ptr,section,bp)
   struct ac_drive *drive_ptr;
   int section;
   struct buf *bp;

{
   int error = 0;
   int mode_err;
   struct ac_device *ac_device_ptr = drive_ptr->ac_device_ptr;

#ifndef _WSIO
   dev_t drive_dev = makedev(drive_maj_blk, makeminor(0,drive_ptr->logical_unit,0,section));
   dev_t surface_dev = makedev(AC_MAJ_BLK,makeminor(drive_ptr->ac_device_ptr->logical_unit,
            section<<4,0,drive_ptr->surface));
#else
   dev_t drive_dev = makedev(drive_maj_blk, ac_minor(ac_card(drive_ptr->ac_device_ptr->device),
            drive_ptr->scsi_address,0,0));
#endif

#ifdef __hp9000s700
   dev_t surface_dev = makedev(AC_MAJ_BLK, MAKE_SURFACE_MINOR(
            ac_card(drive_ptr->ac_device_ptr->device),
            m_busaddr(drive_ptr->ac_device_ptr->device),
            drive_ptr->surface));
#endif
#ifdef __hp9000s300
   dev_t surface_dev = makedev(AC_MAJ_BLK,makeminor(
            m_selcode(drive_ptr->ac_device_ptr->device),
            m_busaddr(drive_ptr->ac_device_ptr->device)<<4,
            0,drive_ptr->surface));
#endif

   if (AC_DEBUG & PMOUNT)
      msg_printf1("pmounting section %d\n",section); 
   
   /* need to open sections other than the opened during spinup */
#ifndef _WSIO
   if (section != ENTIRE_SURFACE) {
      if (AC_DEBUG & PMOUNT)
         msg_printf2("opening the drive in pmount_section, %d,%d\n",drive_ptr->logical_unit,
                     drive_ptr->opened);
      error = (*cdevsw[drive_maj_char].d_open)(makedev(drive_maj_char,
            makeminor(0,drive_ptr->logical_unit,0,section)));
   }
#else
   if (section != ENTIRE_SURFACE) {
      if (AC_DEBUG & PMOUNT)
         msg_printf2("opening the drive in pmount_section, %d,%d\n",drive_ptr->scsi_address,
                     drive_ptr->opened);
      error = (*cdevsw[drive_maj_char].d_open)(makedev(drive_maj_char,
            ac_minor(ac_card(drive_ptr->ac_device_ptr->device),
            drive_ptr->scsi_address,0,0)),0);
   }
#endif

   if (bp != NULL) {
      while (ac_device_ptr->pmount_lock & IN_MOUNT) {
         ac_device_ptr->pmount_lock |= WANT_PMOUNT;
         sleep(&(ac_device_ptr->pmount_lock),PRIBIO);
      }
      ac_device_ptr->pmount_lock |= IN_PMOUNT;
  
      if (AC_DEBUG & MP_IOCTL)
         msg_printf("pmount_section calling autoch_set_drive_mode\n");

      mode_err = autoch_set_drive_mode(drive_ptr, section);

      if (drive_ptr->ac_device_ptr->elements[drive_ptr->surface][section]->pmount != NULL) {
         /* call the pmount code */
         /* drive_ptr->drive_state = FLUSH; */
         error = drive_ptr->ac_device_ptr->elements[drive_ptr->surface][section]->pmount(
            drive_ptr->ac_device_ptr->elements[drive_ptr->surface][section]->mount_ptr,
            drive_dev,surface_dev,bp);
         
         }
      if (error)
         panic("pmount failed");
      /* wakeup anyone waiting for the mount structures */
      if (ac_device_ptr->pmount_lock & (WANT_MOUNT | WANT_PMOUNT))
         wakeup((caddr_t)&(ac_device_ptr->pmount_lock));
      ac_device_ptr->pmount_lock &=  ~(WANT_PMOUNT | IN_PMOUNT);
   }
}

/*
 * Performs a physical unmount of a section on a surface.
 */

void
punmount_section(drive_ptr,section,bp)
   struct ac_drive *drive_ptr;
   int section;
   struct buf *bp;

{
   int error = 0;
   struct ac_device *ac_device_ptr = drive_ptr->ac_device_ptr;
   struct ac_deadlock_struct *dlp;
   void start_async_requests();
   struct timeval atv;
#ifndef _WSIO
   dev_t drive_dev = makedev(drive_maj_blk, makeminor(0,drive_ptr->logical_unit,0,section));
   dev_t surface_dev = makedev(AC_MAJ_BLK,makeminor(drive_ptr->ac_device_ptr->logical_unit,
            section<<4,0,drive_ptr->surface));
#else
   dev_t drive_dev = makedev(drive_maj_blk, 
                             ac_minor(ac_card(drive_ptr->ac_device_ptr->device),
                                       drive_ptr->scsi_address,0,0));
#endif
#ifdef __hp9000s700
   dev_t surface_dev = makedev(AC_MAJ_BLK, MAKE_SURFACE_MINOR(
            ac_card(drive_ptr->ac_device_ptr->device),
            m_busaddr(drive_ptr->ac_device_ptr->device),
            drive_ptr->surface));
#endif
#ifdef __hp9000s300
   dev_t surface_dev = makedev(AC_MAJ_BLK,makeminor(
            m_selcode(drive_ptr->ac_device_ptr->device),
            m_busaddr(drive_ptr->ac_device_ptr->device)<<4,
            0,drive_ptr->surface));

#endif
   if (AC_DEBUG & PMOUNT)
      msg_printf1("unmounting surface %d\n",drive_ptr->surface); 
   if (bp != NULL) {
      while (ac_device_ptr->pmount_lock & IN_MOUNT ) {
         ac_device_ptr->pmount_lock |= WANT_PUNMOUNT;
         sleep(&(ac_device_ptr->pmount_lock),PRIBIO);
      }
      ac_device_ptr->pmount_lock |= IN_PUNMOUNT;
   
      if (drive_ptr->ac_device_ptr->elements[drive_ptr->surface][section]->punmount != NULL) {
         drive_ptr->drive_state = FLUSH;
         ac_device_ptr->deadlocked_drive_ptr = drive_ptr;
   
         /* There might be a waiting request that must be processed before freeze_dev
         will continue.  Get these finished before calling punmount. */
         start_async_requests(drive_ptr,drive_ptr->surface);
   
         GET_TIME(atv);
         /* msg_printf("starting ufs_punmount %d\n",atv.tv_sec); */
         /* call the pmount code */

#ifdef KERNEL_DEBUG_ONLY
     if (force_error.always_deadlock) {
         /* set the deadlock structure to always go through recovery sequence */
         dlp = &(ac_device_ptr->deadlock);
         dlp->flags |= DEADLOCKED;
      }
#endif KERNEL_DEBUG_ONLY
         error = drive_ptr->ac_device_ptr->elements[drive_ptr->surface][section]->punmount(
            drive_ptr->ac_device_ptr->elements[drive_ptr->surface][section]->mount_ptr,
            drive_dev,surface_dev,drive_ptr->ac_device_ptr->xport_bp,
            &(drive_ptr->ac_device_ptr->deadlock));
         }
         GET_TIME(atv);
         /* msg_printf("ended ufs_punmount %d\n",atv.tv_sec); */
         ac_device_ptr->deadlocked_drive_ptr = NULL;

         dlp = &(ac_device_ptr->deadlock);
         if (dlp->flags & DL_TIMER_ON) {
            untimeout(dlp->recover_func,(caddr_t)dlp); 
            dlp->flags &= ~DL_TIMER_ON;
         }
   
      if (ac_device_ptr->pmount_lock & (WANT_MOUNT | WANT_PUNMOUNT))
         wakeup((caddr_t)&(ac_device_ptr->pmount_lock));
      ac_device_ptr->pmount_lock &= ~(WANT_PUNMOUNT | IN_PUNMOUNT);
   }

   /* close the drive */
   /* an ioctl move to a drive closes the drive so don't try to close it again */

#ifndef _WSIO
   if (AC_DEBUG & PMOUNT)
      msg_printf1("Trying to close the drive,%d\n",drive_ptr->logical_unit); 
   if (section != ENTIRE_SURFACE) {
      if (AC_DEBUG & PMOUNT)
         msg_printf2("Closing the drive in punmount_section,%d,%d\n",
            drive_ptr->logical_unit,drive_ptr->opened);
      (*cdevsw[drive_maj_char].d_close)(makedev(drive_maj_char,
            makeminor(0,drive_ptr->logical_unit,0,section)));
      if (AC_DEBUG & PMOUNT)
         msg_printf1("Closed the section = %d\n",section); 
   }
#else
   if (AC_DEBUG & PMOUNT)
      msg_printf1("Trying to close the drive,%d\n",drive_ptr->scsi_address); 
   if (section != ENTIRE_SURFACE) {
      /* this should not be true since s700 does not support sections */
      (*cdevsw[drive_maj_char].d_close)(makedev(drive_maj_char,
               ac_minor(ac_card(drive_ptr->ac_device_ptr->device),
                        drive_ptr->scsi_address,0,0)),0);
      if (AC_DEBUG & PMOUNT)
         msg_printf1("Closed the section = %d\n",section); 
   }
#endif

   else {  
      /* For all systems the ENTIRE_SURFACE is being unmounted */
      /* if the ENTIRE_SECTION is to be closed and some ioctl's have been issued to */
      /* set the drive in write with verify or write without erase mode */
      /* then those flags must be turned off just in case other requests */
      /* access the surface before the delayed close has a chance to  */
      /* do a final close on the surface and clear the flags via a final close */
      /* to the disk driver. */
      
      autoch_clear_drive_state(drive_dev, drive_ptr);
   }

   if (error)
      panic("punmount failed");
}


/*
 * Performs the physical unmount of the surface.  If the
 * file system has set the punmount functions, call them 
 * as well.  This routine is called for every surface
 * before it is removed from a drive.
 * The autochanger must have a cartridge in the specified drive.
 */

void
punmount_surface(drive_ptr,bp)
   struct ac_drive *drive_ptr;
   struct buf *bp;

{
   int i;
   int surface = drive_ptr->surface;

   if (drive_ptr->opened) {
      /* The ioctls might have closed the drive */
      for (i=0;i<=MAX_SECTION;i++) {
         if (drive_ptr->ac_device_ptr->elements[surface][i]) {
            punmount_section(drive_ptr,i,bp);
         }
      }
      /* Must set the drive state to MOVE_OUT so that if during the close,
         another request makes it through, it will get delayed, rather than
         being executed after the close, which will panic! */
      drive_ptr->drive_state = MOVE_OUT;

#ifndef _WSIO
      autoch_clear_phys_drive(makedev(drive_maj_char, makeminor(0,drive_ptr->logical_unit,0,2)),drive_ptr,PUNMOUNT);
#else
      autoch_clear_phys_drive(makedev(drive_maj_blk, ac_minor(ac_card(drive_ptr->ac_device_ptr->device),
                                      drive_ptr->scsi_address,0,0)),drive_ptr,PUNMOUNT);
#endif

      /* must close to match the open on spinup */
#ifndef _WSIO
      if (AC_DEBUG & PMOUNT) {
         msg_printf2("closing the drive in punmount_surface,%d,%d\n",drive_ptr->logical_unit,drive_ptr->opened);
      }
      (*cdevsw[drive_maj_char].d_close)(makedev(drive_maj_char,
            makeminor(0,drive_ptr->logical_unit,0,2)));
      if (AC_DEBUG & PMOUNT) {
         msg_printf2("closed the drive in punmount_surface,%d,%d\n",drive_ptr->logical_unit,drive_ptr->opened);
      }
#else
      if (AC_DEBUG & PMOUNT)
         msg_printf2("closing the drive in punmount_surface,%d,%d\n",drive_ptr->scsi_address,drive_ptr->opened);
      (*cdevsw[drive_maj_char].d_close)(makedev(drive_maj_char,
                ac_minor(ac_card(drive_ptr->ac_device_ptr->device),
                         drive_ptr->scsi_address,0,0)),0);
      if (AC_DEBUG & PMOUNT)
         msg_printf2("closed the drive in punmount_surface,%d,%d\n",drive_ptr->scsi_address,drive_ptr->opened);
#endif
   }
}

/*
 * This is the routine that gets called if the delayed close timeout
 * expires.
 * 
 */

void
wait_close_expired(drive_ptr)
   struct ac_drive *drive_ptr;
{
   struct xport_request *return_reqp;
   int pri;
   extern struct xport_request *request_waiting_for_drive();

   /* critical section */
   pri = spl6();
   if (drive_ptr->close_request == 0) {
      /* Race condition, someone already took care of the close delay */
      splx(pri);
      return;
   }

   if (request_waiting_for_drive(drive_ptr) != NULL) {
      /* There is a request in the xport queue waiting for */
      /* this drive.  Don't do anything. */
      if (AC_DEBUG & DELAYED_CLOSE)
         msg_printf0("Wait close expired with request waiting\n");
      splx(pri);
      return;
   }

   drive_ptr->drive_state = DAEMON_CALL;
   drive_ptr->ac_device_ptr->transport_busy = 1;
   if (AC_DEBUG & DELAYED_CLOSE)
      msg_printf1("close surface delay expired in drive el %d\n",
         drive_ptr->element_address);
   return_reqp = drive_ptr->close_request;
   return_reqp->move_request.destination = return_reqp->move_request.source;
   return_reqp->move_request.source = drive_ptr->element_address;
   /* the invert bit and transport should remain the same */
 
   drive_ptr->surface = ELTOSUR(return_reqp->move_request.destination,
            drive_ptr->ac_device_ptr->storage_element_offset,
            return_reqp->move_request.invert);
   drive_ptr->surface_inproc = EMPTY;
   drive_ptr->close_request = NULL;
   drive_ptr->hog_time_start = 0;  /* set the drive to expired */

   /* move the delayed request to the xport_queue */
   ADD_TO_HEAD_OF_LIST(drive_ptr->ac_device_ptr->xport_queue_head,return_reqp);
   wakeup(&(drive_ptr->ac_device_ptr->xport_queue_head));

   splx(pri);

}

/*
 * This routine checks to see if an element is delayed in a drive.
 * Returns the drive_ptr if it is in a drive, else returns NULL.
 *
 */

struct ac_drive *
element_delayed_in_drive(ac_device_ptr,element)
   struct ac_device *ac_device_ptr;
   int element;
{
   int i;
   struct ac_drive *temp_drive_ptr;
   
   /* msg_printf0("Entering element_deleyed_in_drive\n"); */
   /* loop thru all the drives */
   for (i = 0; i < ac_device_ptr->number_drives; i++) {
      /* msg_printf1("i = %d\n",i); */
      temp_drive_ptr = &(ac_device_ptr->ac_drive_table[i]);
      /*
      msg_printf1("drive_ptr = %x\n",temp_drive_ptr); 
      if (temp_drive_ptr->close_request) {
         msg_printf0("drive ptr has close request pending\n");
         msg_printf1("close move request.source = %d\n",temp_drive_ptr->close_request->move_request.source);
      }
    */
      if ((temp_drive_ptr->close_request) &&
          (temp_drive_ptr->close_request->move_request.source == element)) {
          msg_printf0("element matched drive ptr\n");
         return(temp_drive_ptr);
      }
   }
   return(NULL);
}

/*
 *  This routine resets all the data structures when a
 *  device jam occurs.
 */

void
reset_closed_surfaces(ac_device_ptr)
   struct ac_device *ac_device_ptr;
{
   int i;
   struct ac_drive *drive_ptr;
   int closed_surface;
   struct timeval atv;

   for (i=0; i<ac_device_ptr->number_drives; i++) {
      drive_ptr = &(ac_device_ptr->ac_drive_table[i]);
      if (drive_ptr->close_request) {
         untimeout(wait_close_expired,(caddr_t)drive_ptr);
         closed_surface = ELTOSUR(drive_ptr->close_request->move_request.source,
            ac_device_ptr->storage_element_offset,
            drive_ptr->close_request->move_request.invert);
         FREE_ENTRY(drive_ptr->close_request,xport_request);     
         drive_ptr->close_request = NULL;
         drive_ptr->surface = closed_surface;
         drive_ptr->surface_inproc = closed_surface;
         drive_ptr->drive_state = DO_OP;
         /* reset the hog time for that surface */
         GET_TIME(atv);
         drive_ptr->hog_time_start = atv.tv_sec;
         if (AC_DEBUG & DELAYED_CLOSE)
               msg_printf2("Reopening delayed close surface %d, in drive el %d\n",
                      closed_surface,drive_ptr->element_address);
      }
   }
}

/*
 * This routines schedules the delayed close surface to 
 * be put back into its slot from the drive.
 *
 */

void
move_delayed_surface_home(ac_device_ptr,drive_ptr,closed_surface)
   struct ac_device *ac_device_ptr;
   struct ac_drive *drive_ptr;
   int closed_surface;

{

   struct xport_request *return_reqp;

   if (AC_DEBUG & DELAYED_CLOSE)
      msg_printf2("Moving home delayed close surface %d, in drive el %d\n",
               closed_surface,drive_ptr->element_address);
   /* adjust the close request into the xport request to move it 
        back to its original slot. */
   drive_ptr->ac_device_ptr->transport_busy = 1;
   drive_ptr->surface = closed_surface;
   return_reqp = drive_ptr->close_request;
   if (AC_DEBUG & DELAYED_CLOSE)
      msg_printf2("Adjusting close_request %x, return request %x\n",
               drive_ptr->close_request,return_reqp);
   return_reqp->move_request.destination = return_reqp->move_request.source;
   return_reqp->move_request.source = drive_ptr->element_address;
   return_reqp->x_flags &= ~X_CLOSE_MOVE;
   if (AC_DEBUG & DELAYED_CLOSE)
      msg_printf1("destination = %x \n",return_reqp->move_request.destination);
   /* the invert bit and transport should remain the same */
   /* move the delayed request to the xport_queue */
   /* put it at the head of the list so it will be processed first. */
   if (AC_DEBUG & DELAYED_CLOSE)
      msg_printf2("Adding to head of list %x, this request %x\n",
               ac_device_ptr->xport_queue_head,return_reqp);
   ADD_TO_HEAD_OF_LIST(ac_device_ptr->xport_queue_head,return_reqp);
   drive_ptr->close_request = NULL;
}

/*
 * Force all delayed close surfaces out of drives
 */

void
flush_close(ac_device_ptr)
    struct ac_device *ac_device_ptr;
{
   int i;
   struct ac_drive *temp_drive_ptr;
   int closed_surface;
   
   /* loop thru all the drives */
   /* Must untimeout all the drives before moving any cartridges */
   for (i = 0; i < ac_device_ptr->number_drives; i++) {
      temp_drive_ptr = &(ac_device_ptr->ac_drive_table[i]);
      if (temp_drive_ptr->close_request) {
         untimeout(wait_close_expired,(caddr_t)temp_drive_ptr);
      }
   }
   /* loop thru all the drives, this time move them home */
   for (i = 0; i < ac_device_ptr->number_drives; i++) {
      temp_drive_ptr = &(ac_device_ptr->ac_drive_table[i]);
      if (temp_drive_ptr->close_request) {
         closed_surface = ELTOSUR(temp_drive_ptr->close_request->move_request.source,
            ac_device_ptr->storage_element_offset,
            temp_drive_ptr->close_request->move_request.invert);
         temp_drive_ptr->surface = closed_surface;
         FREE_ENTRY(temp_drive_ptr->close_request,xport_request);
         temp_drive_ptr->close_request = NULL;
         if (AC_DEBUG & DELAYED_CLOSE)
            msg_printf2("Moving home flush surface %d from drive %x\n",closed_surface,temp_drive_ptr);
         punmount_surface(temp_drive_ptr,NULL);
         if (!(move_medium_home(ac_device_ptr,temp_drive_ptr))) { 
            /* set hog_time_start to zero to set age of the drive to be very old */
            temp_drive_ptr->hog_time_start = 0;
            temp_drive_ptr->drive_state = NOT_BUSY;
            temp_drive_ptr->surface = EMPTY;
         }
      }
   }
}

/*
 *
 * This routine adjusts the xport queue for any possible delayed closes.
 * Returns true if no motion is needed.
 */

int
delayed_close_check(ac_device_ptr)
   struct ac_device *ac_device_ptr;
{
   struct xport_request *xport_reqp,*return_reqp;
   struct ac_drive *drive_ptr;
   struct timeval atv;
   int current_surface, closed_surface;
   int priority;

   /* msg_printf0("Entering delayed close check\n"); */

   xport_reqp = ac_device_ptr->xport_queue_head;
   drive_ptr = match_element(ac_device_ptr,xport_reqp->move_request.destination);

   /* msg_printf1("Drive_ptr = %x\n",drive_ptr); */

   priority = spl5();
   if ((drive_ptr == NULL) ||(drive_ptr->close_request == NULL)) {
      
      /* msg_printf0("drive ptr = null or close request on the driver == null\n"); */
      /* msg_printf1("source of requested move = %d\n",xport_reqp->move_request.source); */

      drive_ptr = element_delayed_in_drive(ac_device_ptr,xport_reqp->move_request.source);

      /* msg_printf1("Drive_ptr = %x\n",drive_ptr); */

      if (drive_ptr) {
         /* an ioctl is trying to move in a cartridge */
         /* to a drive but that cartridge is delayed in */
         /* another drive */

         /* msg_printf0("untimeout close delayed on driver ptr\n"); */

         untimeout(wait_close_expired,(caddr_t)drive_ptr);
         closed_surface = ELTOSUR(drive_ptr->close_request->move_request.source,
            ac_device_ptr->storage_element_offset,
            drive_ptr->close_request->move_request.invert);
         move_delayed_surface_home(ac_device_ptr,drive_ptr,closed_surface);
      } else { 
         /* check to see if the destination is delayed in a drive */
         drive_ptr = element_delayed_in_drive(ac_device_ptr,xport_reqp->move_request.destination);
         
         /* msg_printf1("Checking destination in delayed drive\n"); */
         /* msg_printf1("Drive_ptr = %x\n",drive_ptr); */

         if (drive_ptr) {
            /* an ioctl is trying to move in a cartridge */
            /* to a drive but that cartridge is delayed in */
            /* another drive */
   
            /* msg_printf0("untimeout close delayed on driver ptr\n"); */
   
            untimeout(wait_close_expired,(caddr_t)drive_ptr);
            closed_surface = ELTOSUR(drive_ptr->close_request->move_request.source,
               ac_device_ptr->storage_element_offset,
               drive_ptr->close_request->move_request.invert);
            move_delayed_surface_home(ac_device_ptr,drive_ptr,closed_surface);
            }
         else {
               if (AC_DEBUG & DELAYED_CLOSE)
                  msg_printf0("Delayed close check did nothing.\n");
              }
         }
      splx(priority);
      return(0);
   }
   
   /* clear the wait timer for this drive */
   untimeout(wait_close_expired,(caddr_t)drive_ptr);

   current_surface = ELTOSUR(xport_reqp->move_request.source,
            ac_device_ptr->storage_element_offset,
            xport_reqp->move_request.invert);
   closed_surface = ELTOSUR(drive_ptr->close_request->move_request.source,
            ac_device_ptr->storage_element_offset,
            drive_ptr->close_request->move_request.invert);

   if (current_surface == closed_surface) {
      /* same surface still in drive was delayed */
      FREE_ENTRY(drive_ptr->close_request,xport_request);     
      drive_ptr->close_request = NULL;
      drive_ptr->surface = closed_surface;
      drive_ptr->surface_inproc = closed_surface;
      drive_ptr->drive_state = DO_OP;
      /* reset the hog time for that surface */
      GET_TIME(atv);
      drive_ptr->hog_time_start = atv.tv_sec;
      if (AC_DEBUG & DELAYED_CLOSE)
         msg_printf2("Reopening delayed close surface %d, in drive el %d\n",
                closed_surface,drive_ptr->element_address);
      splx(priority);
      return(1); /* no move necessary */ 
   } else if (xport_reqp->move_request.source == 
      drive_ptr->close_request->move_request.source) {
      /* not the same surface but the same element means the */
      /* flip side of closed surface wanted */
      if (AC_DEBUG & DELAYED_CLOSE)
         msg_printf2("Flipping delayed close surface %d, in drive el %d\n",
                closed_surface,drive_ptr->element_address);
      /* set the drive data structures correctly */
      /* This will cause the xport daemon to do a flip */
      drive_ptr->surface = closed_surface;
      drive_ptr->surface_inproc = current_surface;
      drive_ptr->drive_state = MOVE_OUT;

      FREE_ENTRY(drive_ptr->close_request,xport_request);     
      drive_ptr->close_request = NULL;
      splx(priority);
      return(0);
   } else {

      msg_printf0("final else clause, moving surface home\n");

      splx(priority);
      move_delayed_surface_home(ac_device_ptr,drive_ptr,closed_surface);
      return(0);
   }
}


/*
 * Moves the desired cartridge into a drive.  If one is already
 * in the drive, it is returned.  
 */

int
ac_drive_io(ac_device_ptr,drive_ptr,xport_reqp,bp)
   struct ac_device *ac_device_ptr;
   struct ac_drive *drive_ptr;
   struct xport_request *xport_reqp;
   struct buf *bp;

{
   int surface;
   int error = 0;
   int error_switch = 0;
   int escapecode   = 0;
   int flip = 0;

   surface = ELTOSUR(xport_reqp->move_request.source,
         ac_device_ptr->storage_element_offset,
         xport_reqp->move_request.invert);

   try

   if(drive_ptr->surface != EMPTY) {  /* remove cart currently in drive */
      /* physically unmount the surface in the drive to remove */
      punmount_surface(drive_ptr,bp);

      if (!(flip = needs_flip(surface,drive_ptr))) {
         if (AC_DEBUG&AC_STATES)
            msg_printf1("%d ac_drive_io: move_medium_home\n",xport_reqp->pid);
         error = move_medium_home(ac_device_ptr,drive_ptr);
         if (error) {
            msg_printf0("AC: move_medium_home failed\n");
            if (AC_DEBUG&NO_PANICS)
               printf("AC: move_medium_home failed\n");
            error_switch = 1;
            escape(error);
         }
      }
   }
   if (AC_DEBUG & XPORT_OP)
      msg_printf1("%d ac_drive_io: move_medium_to_drive\n",xport_reqp->pid);
   drive_ptr->drive_state = MOVE_IN;
   /* move new cartridge into drive */
   if (flip)  {
      /* just flip over the media that is in the drive */
      xport_reqp->move_request.source = xport_reqp->move_request.destination;
      xport_reqp->move_request.invert = 1;
      error = move_medium (ac_device_ptr,&(xport_reqp->move_request));
   }
   else { /* actually have to get the cartridge from a slot */
      if (AC_DEBUG & XPORT_OP)
         msg_printf1("Moving in element %d\n",xport_reqp->move_request.destination);
      error = move_medium (ac_device_ptr,&(xport_reqp->move_request));
   }
   if (error) {
      msg_printf0("AC: move_medium_to_drive failed\n"); 
      msg_printf0("AC: src=%d, ", xport_reqp->move_request.source); 
      msg_printf0("dest=%d, ", xport_reqp->move_request.destination);
      msg_printf0("inv=%d, ",xport_reqp->move_request.invert); 
      msg_printf0("xpt=%d\n", xport_reqp->move_request.transport);
      if (AC_DEBUG&NO_PANICS)
         printf("AC: move_medium_to_drive failed\n");
      error_switch = 2;
      escape(error); 
   }

   return(error);  /*need to add try recover */

recover: {
   if (AC_DEBUG&PANICS)
      panic("Move error in ac_drive_io");
   switch (error_switch) {
   case 2:
   case 1:
      ac_device_ptr->jammed = 1;
      break;
   default:
    msg_printf("Error_switch = %d \n",error_switch);
    panic("Invalid error_switch in ac_drive_io");
   }
   return(escapecode);
   }

}
/* 
 * This routine determines if another request in the xport queue
 * is waiting to use the drive.  The xport request that is waiting
 * for the drive is returned.
 * 
 */

struct xport_request *
request_waiting_for_drive(drive_ptr)
struct ac_drive *drive_ptr;
{
   struct xport_request *xport_reqp,*waiting_reqp;
   struct ac_device *ac_device_ptr = drive_ptr->ac_device_ptr;

   int priority;
   priority = spl6();
   waiting_reqp = NULL;
   xport_reqp = ac_device_ptr->xport_queue_head;
   if (xport_reqp != NULL) {
      do {
         if ((xport_reqp->sreqp) && (xport_reqp->sreqp->drive_ptr == drive_ptr)) {
            waiting_reqp = xport_reqp;
            break;
            }
         xport_reqp = xport_reqp->forw;
      } while (xport_reqp != ac_device_ptr->xport_queue_head);
   }
   splx(priority);
   return(waiting_reqp);
}

/*
 * Call the async operation.  For the 800, the logical unit number
 * is passed in rather than the scsi_address.
**
** The makedev call in this routine is used as a bit so it can be
** or'ed in with the dev so the correct drive will be used to
** handle the request
*/

void
call_async_op(sreqp,scsi_addr)
   struct surface_request *sreqp;
   int scsi_addr;
{
   struct buf *bp;
#ifndef _WSIO
   dev_t addr_dev = makedev(0,makeminor(0,scsi_addr,0,0));
#else
   dev_t addr_dev = makedev(0,ac_minor(0,scsi_addr,0,0));
#endif

   if (sreqp->op != ac_noop) {
      /* only do it if it is not a noop */
      bp = sreqp->bp;
      bp->b_dev |= addr_dev;

      /* send the request to the scsi driver through the
     switch table */
      (*bdevsw[major(bp->b_dev)].d_strategy)(bp);
   }

}


/*
 * This is the transport daemon.  The autochanger it is to 
 * operate on is passed in as the input parameter.
 * All move operations are performed through the daemon.
 */
 
void
xport_daemon(ac_device_ptr)
   struct ac_device *ac_device_ptr;
{
   int error_switch;
   struct ac_drive *dest_drive_ptr, *src_drive_ptr;
   struct xport_request *xport_reqp,*close_reqp,*waiting_xport;
   struct surface_request *sreqp,*new_sreqp;
   int surface;
   int error;
   int just_moving;
   int pri;
   extern struct buf *geteblk();
   extern void prepare_for_next_request();


ac_device_ptr->xport_bp = geteblk(PMOUNT_BUF_SIZE);

for (; ;) {  /* never exit from the daemon */
   while (ac_device_ptr->xport_queue_head != NULL)  {
      if (AC_DEBUG & XPORT_OP) 
         msg_printf1("processing xport request %x\n",ac_device_ptr->xport_queue_head); 
      if (AC_DEBUG & QUEUES) {
         msg_printf0("XPORT");
         print_xport_queue(ac_device_ptr->xport_queue_head);
      }

      xport_reqp = ac_device_ptr->xport_queue_head; 
      if (xport_reqp->x_flags & X_KILL_XPORTD) {
         if (AC_DEBUG & XPORTD_KILL)
            msg_printf1("xport_daemon(): deleting xport suicide request %x\n",xport_reqp);
         DELETE_FROM_LIST(ac_device_ptr->xport_queue_head,xport_reqp);
         wakeup(xport_reqp);
         exit();
      }

      if (ac_device_ptr->jammed) {
         /* need to flush out the queue here except for reset.*/
         if (xport_reqp->x_flags & X_RESET_AC) {
            /* Process attempting to reset autochanger after jam */
            error = reset_data_structures(ac_device_ptr);
            if (error) { 
               xport_reqp->error = error;
               xport_reqp->x_flags |= X_ERROR;
            }
         } else {
            xport_reqp->error = EIO;
            xport_reqp->x_flags |= X_ERROR;
         }
         if (AC_DEBUG & XPORT_OP) 
            msg_printf1("Deleting xport request %x\n",xport_reqp); 
         DELETE_FROM_LIST(ac_device_ptr->xport_queue_head,xport_reqp); 
         if (AC_DEBUG & XPORT_OP) 
            msg_printf1("waking up xport request %x\n",xport_reqp); 
         wakeup(xport_reqp);
         continue;
      }

      if (xport_reqp->x_flags & X_FLUSH_CLOSE) {
         if (AC_DEBUG & XPORT_OP) 
            msg_printf1("Deleting flush close xport request %x\n",xport_reqp); 
         DELETE_FROM_LIST(ac_device_ptr->xport_queue_head,xport_reqp); 
         flush_close(ac_device_ptr);
         xport_reqp->x_flags |= X_DONE;
         if (AC_DEBUG & XPORT_OP) 
            msg_printf1("waking up xport request %x\n",xport_reqp); 
         wakeup(xport_reqp);
         continue;  /* done with this request, get the next one */
      }
      
      if (delayed_close_check(ac_device_ptr)) {
         /* Don't have to do any moves */
         xport_reqp = ac_device_ptr->xport_queue_head; 
         xport_reqp->x_flags |= X_DONE;
         if (AC_DEBUG & XPORT_OP) 
            msg_printf1("Deleting delayed close check no move xport request %x\n",xport_reqp); 
         DELETE_FROM_LIST(ac_device_ptr->xport_queue_head,xport_reqp); 
         if (xport_reqp->x_flags & X_ASYNC) {
            sreqp = xport_reqp->sreqp;
#ifndef _WSIO
            call_async_op(sreqp,sreqp->drive_ptr->logical_unit);
#else /* s300 and s700 */
            call_async_op(sreqp,sreqp->drive_ptr->scsi_address);
#endif
            sreqp->drive_ptr->drive_state = DO_OP;
            surface = ELTOSUR(xport_reqp->move_request.source,
               ac_device_ptr->storage_element_offset,
               xport_reqp->move_request.invert);
            dest_drive_ptr = match_element(ac_device_ptr,xport_reqp->move_request.destination);
            start_async_requests(dest_drive_ptr,surface);
            if (AC_DEBUG & DELAYED_CLOSE)
               msg_printf0("Xport daemon: calling prep for next after reopened delay\n");
            if (AC_DEBUG & XPORT_OP) 
               msg_printf1("Freeing xport request %x\n",xport_reqp); 
            FREE_ENTRY(xport_reqp,xport_request);
            prepare_for_next_request(sreqp);
         }
         else {
            if (AC_DEBUG & XPORT_OP) 
               msg_printf1("waking up xport request %x\n",xport_reqp); 
            wakeup(xport_reqp);
            }
         continue;  /* start the next loop  */
      }
      /* need to get the head again in case close delay changed it */
      xport_reqp = ac_device_ptr->xport_queue_head; 
      if (AC_DEBUG & XPORT_OP) 
         msg_printf1("Deleting xport request %x\n",xport_reqp); 
      DELETE_FROM_LIST(ac_device_ptr->xport_queue_head,xport_reqp); 
      dest_drive_ptr = match_element(ac_device_ptr,xport_reqp->move_request.destination);
      if (dest_drive_ptr) {  /* destination is a drive */
         surface = ELTOSUR(xport_reqp->move_request.source,
            ac_device_ptr->storage_element_offset,
            xport_reqp->move_request.invert);
         just_moving = 0;
         error = ac_drive_io(ac_device_ptr,dest_drive_ptr,xport_reqp,ac_device_ptr->xport_bp);
         if (error)
            reset_closed_surfaces(ac_device_ptr);
      }
      else { /* destination not a drive, either closing a surface or a move ioctl */
         just_moving = 1;
         src_drive_ptr = match_element(ac_device_ptr,xport_reqp->move_request.source);
         if (src_drive_ptr) { /* source is a drive */
            if ((xport_reqp->x_flags & X_CLOSE_SURFACE) &&
                (ac_device_ptr->ac_close_time != 0)) {
               /* delay the close */
               close_reqp = init_xport_request();
               close_reqp->move_request.source = xport_reqp->move_request.destination;
               close_reqp->move_request.invert = xport_reqp->move_request.invert;
               close_reqp->move_request.destination = 0;
               close_reqp->x_flags = X_CLOSE_MOVE;
               src_drive_ptr->close_request = close_reqp;
               src_drive_ptr->surface = EMPTY;
               src_drive_ptr->surface_inproc = EMPTY;
               src_drive_ptr->drive_state = NOT_BUSY;
               timeout(wait_close_expired,(caddr_t)src_drive_ptr,
                  HZ * src_drive_ptr->ac_device_ptr->ac_close_time);
            }
            else {
               punmount_surface(src_drive_ptr,ac_device_ptr->xport_bp);
               /* don't call move_medium_home because it might be for
                  an ioctl that whats to move the cart to a new slot */
               src_drive_ptr->drive_state = MOVE_OUT;
               error = move_medium(ac_device_ptr,&(xport_reqp->move_request));
               /* set hog_time_start to zero to set age of the drive to be very old */
               if (!error) {
                  src_drive_ptr->hog_time_start = 0;
                  if ((waiting_xport = request_waiting_for_drive(src_drive_ptr)) != NULL) {
                     /* don't set to NOT_BUSY otherwise another request could
                        grab the drive with results in two carts being moved
                        into the same drive. */
                     src_drive_ptr->drive_state = DAEMON_CALL;
                     if (waiting_xport->sreqp) {
                        if (AC_DEBUG & XPORT_OP) 
                           msg_printf0("DID FIX\n");
                        src_drive_ptr->surface_inproc = waiting_xport->sreqp->surface;
                     } else {
                        if (AC_DEBUG & XPORT_OP) 
                           msg_printf0("DIDNT FIX\n");
                     }
                     src_drive_ptr->surface = EMPTY;
                  } else {
                     src_drive_ptr->drive_state = NOT_BUSY;
                     src_drive_ptr->surface = EMPTY;
                     if (xport_reqp->x_flags & X_CLOSE_MOVE) {
                        /* This is to start up any requests */
                        /* which came in during MOVE_OUT    */
                        if (AC_DEBUG & DELAYED_CLOSE) {
                           msg_printf0("xport_daemon(): kicking off possible hung requests\n");
                           msg_printf1("surface_inproc = %d\n",src_drive_ptr->surface_inproc);
                        }
                        pri = spl6();
                        new_sreqp = get_next_request (src_drive_ptr,src_drive_ptr->ac_device_ptr);
                        startup_request(new_sreqp,NULL);
                        splx(pri);
                     }
                  } 
               }
            } 
         } 
         else
            error = move_medium(ac_device_ptr,&(xport_reqp->move_request));
      }
      if (error) {
         xport_reqp->error = error;
         xport_reqp->x_flags |= X_ERROR;
      }


      /* Remove request from queue */
      if (AC_DEBUG & QUEUES) {
         msg_printf0("XPORT");
         print_xport_queue(ac_device_ptr->xport_queue_head);
      }
      if (AC_DEBUG & QUEUES) 
         print_xport_queue(ac_device_ptr->xport_queue_head);
   
      if ((just_moving == 0) && (!error)) {
         /* call the spinup daemon */
         xport_reqp->drive_ptr = dest_drive_ptr;
         xport_reqp->surface = surface;
         /* on a no_op request which resulted from a deadlock, */
         /* the drive_ptr in the sreqp may not be correct. */
         /* Therefore, it must to changed to match the element it */
         /* was moved to so it won't try to execute the request */
         /* to the wrong drive resulting in a panic. */

         if (xport_reqp->sreqp)
            xport_reqp->sreqp->drive_ptr = dest_drive_ptr;
         if (xport_reqp->surface < 0)
            panic("xport_daemon: invalid surface to spinup");
         if (AC_DEBUG & QUEUES) {
            msg_printf0("SPINUP ");
            print_spinup_queue(ac_device_ptr->spinup_queue_head);
         }
         ADD_TO_LIST(ac_device_ptr->spinup_queue_head,xport_reqp);
         if (AC_DEBUG & QUEUES) 
            print_spinup_queue(ac_device_ptr->spinup_queue_head);
         if (AC_DEBUG & XPORT_OP) 
            msg_printf1("waking up xport request for spinup queue %x\n",xport_reqp); 
         wakeup(&(ac_device_ptr->spinup_queue_head));
      }

      if (xport_reqp->x_flags & X_ASYNC) {
         if (AC_DEBUG & XPORT_OP)
            msg_printf0("Xport_daemon: async request processed \n");
         if (error) {
            if (reset_data_structures(ac_device_ptr) != 0) {
               pri = spl6();
               if ((sreqp = ac_device_ptr->sreqq_head) != NULL) {
                  do {
                     if (sreqp->surface == xport_reqp->sreqp->surface &&
                         sreqp != xport_reqp->sreqp) {
                        DELETE_FROM_LIST(ac_device_ptr->sreqq_head, sreqp);
                        sreqp->bp->b_error = error;
                        sreqp->bp->b_flags |= B_ERROR;
                        iodone(sreqp->bp);
                     }
                     sreqp = sreqp->forw;
                  } while (sreqp != ac_device_ptr->sreqq_head);
               }
               (void) splx(pri);
            }
            xport_reqp->sreqp->bp->b_error = error;
            xport_reqp->sreqp->bp->b_flags |= B_ERROR;
            iodone(xport_reqp->sreqp->bp); 
            prepare_for_next_request(xport_reqp->sreqp);
            if (AC_DEBUG & XPORT_OP) 
               msg_printf1("Error, freeing  %x\n",xport_reqp); 
            FREE_ENTRY(xport_reqp,xport_request);
         }
      }
      else {
         if (just_moving || error) {
            if (!(xport_reqp->x_flags & X_ERROR))
               xport_reqp->x_flags |= X_DONE;
            if (xport_reqp->x_flags & X_NO_WAITING) {
               if (AC_DEBUG & XPORT_OP)
                  msg_printf0("Xport_daemon:Killed entry\n");
               }
            /* wakeup any ioctls or close surface sleeping on the request */
            if (AC_DEBUG & XPORT_OP) 
               msg_printf1("waking up xport request %x\n",xport_reqp); 
            wakeup(xport_reqp);
            /* if this was request caused by delayed close, free it here */
            if (xport_reqp->x_flags & X_CLOSE_MOVE) {
               if (AC_DEBUG & DELAYED_CLOSE)
                  msg_printf1("xport_daemon(): freeing delayed close request 0x%X\n", xport_reqp);
               FREE_ENTRY(xport_reqp,xport_request);
            }
         /* The spinup daemon will wakeup requests for drive io */
         }
      }
      msg_printf0("end of xport loop\n");
   } /* end while */

   ac_device_ptr->transport_busy = 0;
   /* sleep until new request */
   sleep(&(ac_device_ptr->xport_queue_head),PRIBIO);  /* don't want signal to wakeup daemon */

}   /* don't ever return */

} /* end xport_daemon */


/*
 * Flushes out the waiting async
 * requests for the surface.
 * Returns 1 is started anything, else 0;
 * The surface should already be in the drive
 * specified by the drive_ptr.
 */

void
start_async_requests(drive_ptr,surface)
   struct ac_drive *drive_ptr;
   int surface;
{
   struct surface_request *sreqp,*tmp_sreqp;
   struct ac_device *ac_device_ptr = drive_ptr->ac_device_ptr;

   int priority; 
   /* keep out all interrupts that might modify the list */ 
   priority = spl6(); 
   /* loop thru the queue */ 
   /* print_surface_queue(ac_device_ptr); */
   sreqp = ac_device_ptr->sreqq_head; 
   if (sreqp != NULL) { 
      do {
         if ((sreqp->r_flags & R_ASYNC) && (sreqp->surface == surface) &&
            !(sreqp->r_flags & R_BUSY)) {  /* don't startup the one that started it all */

            sreqp->drive_ptr = drive_ptr;

            /* increment reference count */
            (drive_ptr->ref_count)++;
         
            /* mark request busy */
            sreqp->r_flags |= R_BUSY;
         
            /* for same surface and async request */
            if (AC_DEBUG & OPERATION) {
               msg_printf1("Start_async: starting request for pid %d\n",sreqp->pid);
               msg_printf1("Calling scsi strategy with bp = %x\n",sreqp->bp);
               msg_printf2("Head = %x, sreqp = %x\n",ac_device_ptr->sreqq_head,sreqp);
            }
#ifndef _WSIO
            call_async_op(sreqp,drive_ptr->logical_unit);
#else /* s300 and s700 */
            call_async_op(sreqp,drive_ptr->scsi_address);
#endif
            /* decrement reference count */
            (drive_ptr->ref_count)--;
            /* can't use the macros FOR because the entry is being deleted */
            tmp_sreqp = sreqp;
            sreqp = sreqp->forw;
            DELETE_FROM_LIST(ac_device_ptr->sreqq_head,tmp_sreqp);
            FREE_ENTRY(tmp_sreqp,surface_request);
            if (ac_device_ptr->sreqq_head == NULL)
               sreqp = NULL;
         } else {
            sreqp = sreqp->forw;
         }
      } while (sreqp != ac_device_ptr->sreqq_head); 
   } 
   splx(priority); 
}
   

/*
 * Gets the surface ready to use.  Flushes out the waiting async
 * requests for the surface as well.
 */

int
prepare_surface_for_use(drive_ptr,surface,bp,drive_state)
   struct ac_drive *drive_ptr;
   int surface;
   struct buf *bp;
   enum drive_states drive_state;
{
   int error;
   int i;
   
   if (AC_DEBUG & PMOUNT)
      msg_printf1("Prepareing surface %d for use\n",surface); 
   if ((error = do_spinup(drive_ptr,surface)) != 0) {
      printf("Surface %d FAILED PREPARE FOR USE.\n",surface);
#ifndef _WSIO
         printf("autochanger is logical unit number %d \n",drive_ptr->ac_device_ptr->logical_unit);
#endif
      msg_printf1("Surface %d FAILED PREPARE FOR USE.\n",surface);
   }
   else {
      for (i=0;i<=MAX_SECTION;i++) {
         if (drive_ptr->ac_device_ptr->elements[surface][i])
            pmount_section(drive_ptr,i,bp);
      }
      drive_ptr->drive_state = drive_state;
     
      if (AC_DEBUG & PMOUNT)
         msg_printf1("Surface %d ready for use.\n",surface); 
   }
   return(error);
 
}
 
/*
 * This routine returns the first sreqp in the queue
 * that is not for the current surface that is deadlocked
 * or not one set to noop.  Note, this may return requests
 * that are marked busy.
 */

struct surface_request *
grab_first_real_request(ac_device_ptr,deadlocked_surface)
   struct ac_device *ac_device_ptr;
   int deadlocked_surface;
{
   struct surface_request *sreqp;
   struct ac_drive *drive_ptr;

   FOR_EACH_SWAP_REQUEST
      if ((sreqp->op != ac_noop) &&  /* can't be a previous noop */
         (sreqp->r_flags & R_ASYNC)) { /* cant be a sync request, the op_parms are on different stack */
         /* it might be for a surface in another drive */
         if (!(current_sur_inserted(ac_device_ptr,sreqp->surface,&drive_ptr))) {
            if (AC_DEBUG & DEADLOCK) {
               msg_printf1("grabbed sreqp = %x\n",sreqp);
               msg_printf1("surface = %d\n",sreqp->surface);
               msg_printf1("rflags = %x\n",sreqp->r_flags);
               msg_printf1("result = %d\n",sreqp->result);
               msg_printf1("case switch = %x\n",sreqp->case_switch);
               msg_printf1(" AC_DELAY = %d\n",AC_DELAY);
               msg_printf1(" AC_NO_DELAY = %d\n",AC_NO_DELAY);
               msg_printf1(" AC_FLUSH  = %d\n",AC_FLUSH);
               }
            RETURN_FROM_FOR(sreqp);
         } else {
               /* there might be a sreq that is scheduled for another drive
                  but is for the deadlocked surface.  This must be executed now. */
             if (sreqp->surface == deadlocked_surface) {
                /* execute it and set the request to ac_noop */
#ifndef _WSIO
                 call_async_op(sreqp,drive_ptr->logical_unit);
#else
                 call_async_op(sreqp,drive_ptr->scsi_address);
#endif /* ! _WSIO */
                 sreqp->op = ac_noop;
             }
         }
      }
   END_FOR
   if (AC_DEBUG & DEADLOCK)
      msg_printf0("grabbed sreqp = NULL\n");
   return(NULL);
}


struct xport_request *
create_xport_request(surface,drive_ptr)
   int surface;
   struct ac_drive *drive_ptr;

{
   struct xport_request *xport_reqp;
   struct ac_device *ac_device_ptr;

   ac_device_ptr = drive_ptr->ac_device_ptr;
   xport_reqp = init_xport_request(); 
   xport_reqp->move_request.transport = ac_device_ptr->transport_address;
   xport_reqp->move_request.source = SURTOEL(surface,ac_device_ptr->storage_element_offset);
   xport_reqp->move_request.destination = drive_ptr->element_address;
   xport_reqp->move_request.invert = FLIP(surface);
   if (AC_DEBUG & DEADLOCK) {
      msg_printf1("move surface  = %d\n",drive_ptr->surface);
      msg_printf1("move drive state  = %d\n",drive_ptr->drive_state);
      msg_printf1("flush             = %d\n",FLUSH);
      msg_printf1("move source = %d\n",xport_reqp->move_request.source);
      msg_printf1("move dest   = %d\n",xport_reqp->move_request.destination);
      msg_printf1("move flip   = %d\n",xport_reqp->move_request.invert);
   }
   return(xport_reqp);
}

/*
 * Starts up any waiting synch requests.
 */

void
start_sync_requests(drive_ptr,surface)
   struct ac_drive *drive_ptr;
   int surface;
{
   struct surface_request *sreqp;
   struct ac_device *ac_device_ptr=drive_ptr->ac_device_ptr;
   /* loop through all requests looking for same surface */
   FOR_EACH_SWAP_REQUEST
      if (sreqp->surface == surface) {
         if (!(sreqp->r_flags & R_ASYNC)) {
            /* only start sync requests */
            if (AC_DEBUG&DEADLOCK)
               msg_printf1("calling startup_req sreqp is %x\n", sreqp);
            startup_request(sreqp,NULL);
         }
      }
   END_FOR
}


/*
 * This routine attempts to put in a cartridge
 * in the hope the xport daemon will undeadlock.
 */

void
shake_and_bake(ac_device_ptr)
   struct ac_device * ac_device_ptr;

{

   struct surface_request *sreqp;
   struct xport_request *xport_reqp;
   struct ac_drive *target_drive_ptr,*deadlocked_drive_ptr;
   int save_flags;
   int new_surface,deadlocked_surface,return_surface;
   int error = 0;
   enum drive_states save_drive_state;

   if (AC_DEBUG & DEADLOCK)
      msg_printf0("Entering shake_and_bake\n");
   /* determine which drive is deadlocked */

   deadlocked_drive_ptr = ac_device_ptr->deadlocked_drive_ptr; 
   deadlocked_surface = deadlocked_drive_ptr->surface;
   return_surface = deadlocked_surface;
   sreqp = grab_first_real_request(ac_device_ptr,deadlocked_surface);
   if (sreqp == NULL) {
      if (AC_DEBUG & DEADLOCK)
         msg_printf0("Nothing to recover from\n");
      return; /* nothing we can do */
   }
   save_flags = sreqp->r_flags;
   new_surface = sreqp->surface;
   sreqp->r_flags |= R_BUSY; /* mark that request busy */
   /* determine cartridge source */
   if (!(current_element_really_inserted(ac_device_ptr,new_surface,&target_drive_ptr))) {
      /* if the element is not inserted anywhere, set target drive to deadlocked drive */
      target_drive_ptr = deadlocked_drive_ptr;
      if (AC_DEBUG & DEADLOCK) {
         msg_printf0("target_drive_ptr = deadlocked_drive_ptr\n");
      }
   }
   else {
      /* need to use the other drive */
      if (AC_DEBUG & DEADLOCK) {
         msg_printf0("target_drive_ptr != deadlocked_drive_ptr\n");
      }
      return_surface = target_drive_ptr->surface;
   }
   if (AC_DEBUG & DEADLOCK) {
      msg_printf1("deadlocked_surface = %d\n",deadlocked_surface);
      msg_printf1("new_surface = %d\n",new_surface);
      /* printf("deadlocked_surface = %d\n",deadlocked_surface); */
      /* printf("new_surface = %d\n",new_surface); */
   }
   /* save the drive state to set it back when finished. */
   save_drive_state = target_drive_ptr->drive_state;
   xport_reqp = create_xport_request(new_surface,target_drive_ptr);
   /* do the move but not the punmount */
   error = ac_drive_io(ac_device_ptr,target_drive_ptr,xport_reqp,NULL);
   if (error) {
      msg_printf0("ac_drive_io move of deadlocked surface out failed\n");
      panic("ac_drive_io move of deadlocked surface out failed");
   }

   if (AC_DEBUG & DEADLOCK) 
      msg_printf0("exchanged cartridges, spinning up the new one\n");
   error = prepare_surface_for_use(target_drive_ptr,new_surface,ac_device_ptr->spinup_bp,FLUSH);
 
   if (AC_DEBUG & DEADLOCK) 
      msg_printf0("spun up the new one, starting requests\n");
   FREE_ENTRY(xport_reqp,xport_request);
   /* now make sure no other requests get started by upping the ref count of the drive */
   target_drive_ptr->ref_count++;
   start_async_requests(target_drive_ptr,new_surface);
   start_sync_requests(target_drive_ptr,new_surface);

   sreqp->drive_ptr = target_drive_ptr;
   if (sreqp->r_flags & R_ASYNC) {
#ifndef _WSIO
      call_async_op(sreqp,sreqp->drive_ptr->logical_unit);
#else /* s300 and s700 */
      call_async_op(sreqp,sreqp->drive_ptr->scsi_address);
#endif
   }
   else {
#ifndef _WSIO
         error=call_sync_op(sreqp->op,sreqp->op_parmp,sreqp->drive_ptr->logical_unit);
#else /* s300 and s700 */
         error=call_sync_op(sreqp->op,sreqp->op_parmp,sreqp->drive_ptr->scsi_address);
#endif
   }
   sreqp->op = ac_noop;
   sreqp->bp = NULL;
   sreqp->r_flags = save_flags;
   if (AC_DEBUG & DEADLOCK) 
      msg_printf0("started requests, do more moves\n");

   xport_reqp = create_xport_request(return_surface,target_drive_ptr);
   error = ac_drive_io(ac_device_ptr,target_drive_ptr,xport_reqp,NULL);
   if (error) {
      msg_printf0("ac_drive_io move of deadlocked surface back in failed\n");
      panic("ac_drive_io move of deadlocked surface back in failed");
   }
   if (AC_DEBUG & DEADLOCK) 
      msg_printf0("exchanged cartridges, spinning up the new one\n");
   /* want to put the drive state back to what it was before cartridges started to move. */
   /* This will cause a problem if it is not done if the target drive is not equal to the
      deadlocked drive.  The target drive was not set to FLUSH but something else. */
   error = prepare_surface_for_use(target_drive_ptr,return_surface,ac_device_ptr->spinup_bp,save_drive_state);
   if (AC_DEBUG & DEADLOCK) 
      msg_printf0("spun up the new one, starting requests\n");
   FREE_ENTRY(xport_reqp,xport_request);
   /* make sure to flush things here */
   start_async_requests(target_drive_ptr,deadlocked_surface);
   start_sync_requests(target_drive_ptr,deadlocked_surface);
   target_drive_ptr->ref_count--;
   if (AC_DEBUG & DEADLOCK) 
      msg_printf0("exit shake_and_bake\n");
}
 
/*
 *  Actually performs the daemon deadlock recovery sequences.
 *
 */

void
deadlock_recovery(ac_device_ptr)
   struct ac_device *ac_device_ptr;

{
   struct ac_deadlock_struct *dlp;


   /* printf("autochanger deadlock recovery in progress\n"); */
   dlp = &(ac_device_ptr->deadlock);
   if (!(ac_device_ptr->deadlock.flags & RECOVERED)) {
      if (AC_DEBUG & DEADLOCK)
         msg_printf0("Entering deadlock_recovery\n");
      shake_and_bake(ac_device_ptr);
      if (ac_device_ptr->deadlock.flags & RECOVERED) {
         /* printf("RECOVERED 1....\n"); */
         wakeup(dlp);
      }
      else {
         /* printf("Still did not recover, restarting timer\n"); */
         if (dlp->flags & DL_TIMER_ON)
            panic("deadlock_recovery:  Restarting running timer");
         else {
            timeout(dlp->recover_func,(caddr_t)dlp,HZ*DEADLOCK_TIMEOUT); 
            dlp->flags |= DL_TIMER_ON;
         }
      }
   }
   else { /* it might have recovered while the timer was being processed */
      /* printf("RECOVERED 2....\n"); */
      wakeup(dlp);
   }
}

/*
 * This is the spinup daemon.  The autochanger it is to
 * operate on is passed in as the input parameter.
 */
 
void
spinup_daemon(ac_device_ptr)
   struct ac_device *ac_device_ptr;
{
   int error_switch;
   struct ac_drive *drive_ptr;
   struct xport_request *spinup_reqp;
   struct surface_request *sreqp;
   int surface;
   int error;
   int pri;
   extern struct buf *geteblk();
   extern void prepare_for_next_request();
   extern void call_async_op();
 
ac_device_ptr->spinup_bp = geteblk(PMOUNT_BUF_SIZE);

for (; ;) {  /* never exit from the daemon */
   /* pri = spl6(); */


   /* start of main daemon */
   while (ac_device_ptr->spinup_queue_head != NULL)  {
      spinup_reqp = ac_device_ptr->spinup_queue_head; 
      if (spinup_reqp->x_flags & X_RECOVER) {
         if (AC_DEBUG & DEADPANIC) 
            panic("Deadlock detected");
         if (AC_DEBUG & DEADLOCK) 
            msg_printf1("Deleting recovery request %x\n",spinup_reqp); 
         DELETE_FROM_LIST(ac_device_ptr->spinup_queue_head,spinup_reqp); 
         spinup_reqp->x_flags = 0;
         deadlock_recovery(ac_device_ptr);
         continue;  /* done with this request, get the next one */
      }
      drive_ptr = spinup_reqp->drive_ptr;
      surface = spinup_reqp->surface;
      if (AC_DEBUG & SPINUP_OP) {
         msg_printf1("Spinup daemon: Drive = %d",drive_ptr->element_address); 
         msg_printf1("containing surface %d\n",surface); 
         msg_printf1("%d spinup_daemon: do_spinup\n",spinup_reqp->pid);
      }
      if (error = prepare_surface_for_use(drive_ptr,surface,ac_device_ptr->spinup_bp,DO_OP)) {
         /* did it fail? */
         if (error != EINTR) {
            /* assume spinup failed */
            msg_printf0("AC: do_spinup failed\n");
            if (AC_DEBUG&NO_PANICS)
               printf("AC: do_spinup failed\n");
         }
      }
      /* Remove request from queue */
      DELETE_FROM_LIST(ac_device_ptr->spinup_queue_head,spinup_reqp); 
      if (spinup_reqp->x_flags & X_ASYNC) {
         if (AC_DEBUG & SPINUP_OP)
            msg_printf0("Spinup daemon: spinup complete on async \n"); 
         if (error) {
            if (ac_send_reset(ac_device_ptr) != 0) {
               pri = spl6();
               if ((sreqp = ac_device_ptr->sreqq_head) != NULL) {
                  do {
                     if (sreqp->surface == spinup_reqp->sreqp->surface &&
                         sreqp != spinup_reqp->sreqp) {
                        DELETE_FROM_LIST(ac_device_ptr->sreqq_head, sreqp);
                        sreqp->bp->b_error = error;
                        sreqp->bp->b_flags |= B_ERROR;
                        iodone(sreqp->bp);
                     }
                     sreqp = sreqp->forw;
                  } while (sreqp != ac_device_ptr->sreqq_head);
               }
               (void) splx(pri);
            }
            spinup_reqp->sreqp->bp->b_error = error;
            spinup_reqp->sreqp->bp->b_flags |= B_ERROR;
            iodone(spinup_reqp->sreqp->bp); 
            prepare_for_next_request(spinup_reqp->sreqp);
         }
         else { /* need to start up a request */
            if (AC_DEBUG & SPINUP_OP)
               msg_printf0("Spinup daemon: call_async_op for async request \n"); 
            sreqp = spinup_reqp->sreqp;
            sreqp->drive_ptr->drive_state = DO_OP;
            if (AC_DEBUG&AC_STATES)
               msg_printf1("%d spinup daemon: do_op\n",sreqp->pid);
#ifndef _WSIO
            call_async_op(sreqp,sreqp->drive_ptr->logical_unit);
#else /* s300 and s700 */
            call_async_op(sreqp,sreqp->drive_ptr->scsi_address);
#endif
            sreqp->drive_ptr->drive_state = DO_OP;
            start_async_requests(drive_ptr,surface);
            if (AC_DEBUG & SPINUP_OP)
               msg_printf0("Spinup daemon: calling prep for next\n");
            prepare_for_next_request(sreqp);
         }
         FREE_ENTRY(spinup_reqp,xport_request);
      }
      else {
         spinup_reqp->error = error;
         if (error) {
            spinup_reqp->x_flags |= X_ERROR;
            printf("Spinup daemon: Spinup FAILED in\n");
            printf("autochanger at SCSI dev %x \n",
               ac_device_ptr->device);
#ifndef _WSIO
         printf("autochanger is logical unit number %d \n",ac_device_ptr->logical_unit);
#endif
            printf("had drive at SCSI ID (%d) fail to spinup\n",
               spinup_reqp->drive_ptr->scsi_address);
         }
         else {
            spinup_reqp->x_flags |= X_DONE;
            /* ioctl move may not have a surface request */
            if (spinup_reqp->sreqp) {
               drive_ptr->drive_state = DO_OP;
               start_async_requests(drive_ptr,surface);
            }
            if (AC_DEBUG & SPINUP_OP)
               msg_printf0("Spinup daemon: Spinup complete \n"); 
         }
         if (spinup_reqp->x_flags & X_NO_WAITING) {
            if (AC_DEBUG & SPINUP_OP)
               msg_printf0("spinup_daemon:Killed entry\n");
            }
         wakeup(spinup_reqp);
      }

   } /* end while */

   /* sleep until new request */
   sleep(&(ac_device_ptr->spinup_queue_head),PRIBIO);  /* don't want signal to wakeup daemon */
   /* splx(pri); */

} /* loop forever */
} /* end spinup daemon */


/*
 * This procedure returns the queue statistics.
 */
void
get_q_stats (addr,ac_device_ptr)
   caddr_t addr;
   struct ac_device *ac_device_ptr;
{
   int i, surface_request_table[MAX_SUR];
   struct surface_request *sreqp;

   /* zero the array */
   for (i=0; i<MAX_SUR; i++) {
      surface_request_table[i] = 0;
   }

   /* zero the size */
   (((struct queue_stats *) (addr))->size) = 0;

   /* zero the mix */
   (((struct queue_stats *) (addr))->mix) = 0;

   /* get the information from the queue */
   FOR_EACH_SWAP_REQUEST
      /* increment the count for the surface */
      /* (NOTE: surface numbers start at 1) */
      surface_request_table[sreqp->surface - 1]++;

      /* increment the size */
      (((struct queue_stats *) (addr))->size)++;
   END_FOR

   /* calculate the mix */
   for(i=0; i<MAX_SUR; i++) {
      if (surface_request_table[i] != 0) {
         (((struct queue_stats *) (addr))->mix)++;
      }
   }
}

/*
 * This procedure handles the ioctl ACIOC_INITIALIZE_ELEMENT_STATUS.
 */
int
do_ioctl_initialize_element_status (ac_device_ptr)
   struct ac_device *ac_device_ptr;
{
   int error = 0;

   /* the initialize_element_status command can only be executed if
    * it is the only
    * process working on the autochanger.  That means that the open
    * count of surface zero is 1 and all other open_counts are 0.
    */        
   if ((total_surface_count(ac_device_ptr,0) == 3) && /* Assume the daemons must be running */
       (total_open_cnt(ac_device_ptr) == 3)) {
      ac_device_ptr->rezero_lock = REZERO_BUSY;
#ifdef KERNEL_DEBUG_ONLY
      if (force_error.ioctl_rezero_unit) error = EIO;
      else
#endif KERNEL_DEBUG_ONLY
      error = autox0_ioctl(ac_device_ptr->device,AUTOX0_INITIALIZE_ELEMENT_STATUS,0);

      if (error) {
         /* autochanger must have jammed */
         ac_device_ptr->jammed = 1;
      } else {
         /* wait for disk to spin up in case it was taken out
          * while doing this command.
          */
         timeout (wakeup, (caddr_t)ac_device_ptr, HZ*SPINUP_WAIT/1000);
         error = sleep(ac_device_ptr,PSLEP | PCATCH);
         if (error) {
            return(EINTR);
         }
         untimeout (wakeup, (caddr_t)ac_device_ptr);
      }

      if (ac_device_ptr->rezero_lock & REZERO_WANTED) {
         wakeup((caddr_t)&ac_device_ptr->rezero_lock);
      }

      ac_device_ptr->rezero_lock = 0;
   } else {
      return(EBUSY); 
   }
   return(error);
}


/*
 * This procedure handles the ioctl ACIOC_MOVE_MEDIUM.
 */
int
do_ioctl_move_medium (op_parmp, ac_device_ptr)
   struct operation_parms *op_parmp;
   struct ac_device *ac_device_ptr;
{
   int source, destination, invert, error = 0;
   struct ac_drive *drive_ptr_source, *drive_ptr_dest;
   struct xport_request *xport_reqp;
   int pri;

   /* get the source, destination and invert */ 
   source = ((struct move_medium_parms *) (op_parmp->v))->source; 
   destination = ((struct move_medium_parms *) (op_parmp->v))->destination;
   invert = ((struct move_medium_parms *) (op_parmp->v))->invert;

   /* get drive_ptrs for source and destination */
   drive_ptr_dest = match_element(ac_device_ptr,destination);
   drive_ptr_source = match_element(ac_device_ptr,source);

   /* Don't allow moves if source or destination is picker.
    */
   if ((ac_device_ptr->transport_address == source) ||
      (ac_device_ptr->transport_address == destination)) {
      msg_printf("AUTOCH: move to/from picker not allowed\n");
      return(ENXIO);
   }

   /* Don't allow moves if source is mailslot and destination is drive or
    * vice versa , since the mailslot can not be used as a slot.
    */
   if ((ac_device_ptr->mailslot_address == source) &&
      drive_ptr_dest) {
      msg_printf("AUTOCH: move mailslot to drive not allowed\n");
      return(ENXIO);
   }
   if ((ac_device_ptr->mailslot_address == destination) &&
      drive_ptr_source) {
      msg_printf("AUTOCH: move drive to mailslot not allowed\n");
      return(ENXIO);
   }

   /* Only let moves occur if:
    *      - if the source or destination is a medium that is NOT open AND
    *      - if the source or destination is a drive it is NOT open.
    */
   
   /* Don't allow moves from drive to drive. */
   if (drive_ptr_dest && drive_ptr_source) {
      msg_printf("AUTOCH: move from drive to drive not allowed\n");
      return(ENXIO);
   }

   /* if the destination is a drive make sure it is NOT in use */
   if((drive_ptr_dest) && (drive_ptr_dest->inuse)) {
      msg_printf("AUTOCH: move to drive already in use not allowed\n");
      return(ENXIO);
   }

   /* if the source is a drive make sure it is NOT in use*/
   if((drive_ptr_source) && (drive_ptr_source->inuse)) {
      msg_printf("AUTOCH: move from drive already in use not allowed\n");
      return(ENXIO);
   }

   /* if the source is a storage element make sure the associated media
    * is NOT open 
    */
   if(IS_STORAGE(source,ac_device_ptr) &&
      (total_surface_count(ac_device_ptr,
      ELTOSUR(source,ac_device_ptr->storage_element_offset,invert))> 0)) {
      msg_printf("AUTOCH: move from media that is open not allowed\n");
      return(ENXIO);
   }

   /* if the destination is a storage element make sure the associated media
    * is NOT open 
    */
   if(IS_STORAGE(destination,ac_device_ptr) &&
      (total_surface_count(ac_device_ptr,
      ELTOSUR(destination,ac_device_ptr->storage_element_offset,invert))> 0)) {
      msg_printf("AUTOCH: move to slot that is open not allowed\n");
      return(ENXIO);
   }

   if (drive_ptr_source) {
      drive_ptr_source->surface_inproc = EMPTY;
      drive_ptr_source->drive_state = MOVE_OUT;
      if (drive_ptr_source->opened) { /* close it first */
#ifndef _WSIO
      (*cdevsw[drive_maj_char].d_close)(makedev(drive_maj_char,
            makeminor(0,drive_ptr_source->logical_unit,0,2)));
#else
      (*cdevsw[drive_maj_char].d_close)(makedev(drive_maj_char,
               ac_minor(ac_card(drive_ptr_source->ac_device_ptr->device),
                        drive_ptr_source->scsi_address,0,0)),0);
#endif
      drive_ptr_source->opened = 0;
      }
   }
   if (drive_ptr_dest) {
      drive_ptr_dest->surface_inproc = ELTOSUR (source,
                ac_device_ptr->storage_element_offset,
                invert);
      drive_ptr_dest->drive_state = MOVE_IN;
   }
   
   xport_reqp = init_xport_request();
   xport_reqp->move_request.source = source;
   xport_reqp->move_request.destination = destination;
   xport_reqp->move_request.invert = invert;
   ADD_TO_LIST(ac_device_ptr->xport_queue_head,xport_reqp);

   ac_device_ptr->transport_busy = 1;
   pri = spl6();
   wakeup(&(ac_device_ptr->xport_queue_head));
   /* sleep on xport_request until the xport daemon is finished */
   sleep(xport_reqp,PRIBIO);
   (void) splx(pri);
   
   if (!(xport_reqp->x_flags & X_DONE)) { /* move had an error */ 
      error = xport_reqp->error;
      if(!error) 
         panic("ioctl: Failed to complete but gave no error");
      msg_printf1("AUTOCH: move failed, error = %d\n",error);
      return(error);
   }
   FREE_ENTRY(xport_reqp,xport_request);

   if (drive_ptr_dest) {
#ifndef _WSIO
      (*cdevsw[drive_maj_char].d_close)(makedev(drive_maj_char,
            makeminor(0,drive_ptr_dest->logical_unit,0,2)));
#else
      (*cdevsw[drive_maj_char].d_close)(makedev(drive_maj_char,
               ac_minor(ac_card(drive_ptr_dest->ac_device_ptr->device),
                        drive_ptr_dest->scsi_address,0,0)),0);
#endif
      drive_ptr_dest->drive_state = NOT_BUSY;
      drive_ptr_dest->opened = 0;
   }

   if (drive_ptr_source) {
      drive_ptr_source->surface = EMPTY;
      drive_ptr_source->drive_state = NOT_BUSY;
   }
   return(error);
}


/*
 * This procedure is called if the operation is an ioctl.  It cases out
 * the ioctl.
 */
int
do_ioctl (op_parmp, ac_device_ptr)
   struct operation_parms *op_parmp;
   struct ac_device *ac_device_ptr;
{
   int error = 0;
   int pri;
   int surface;
   int i;
   struct xport_request *xport_reqp;
   /* error may be a problem */

   switch (op_parmp->code) {
   case ACIOC_MOVE_MEDIUM: 
      error = do_ioctl_move_medium (op_parmp, ac_device_ptr);
      break;
   case ACIOC_READ_Q_CONST: 
      if (major(op_parmp->dev) == AUTOX0_MAJ_CHAR) {
         surface = 0;
      } else {
         surface = ac_surface(op_parmp->dev);
      }
      ((struct queue_const *) (op_parmp->v))->wait_time = ac_device_ptr->ac_wait_time[surface];
      ((struct queue_const *) (op_parmp->v))->hog_time  = ac_device_ptr->ac_hog_time[surface];
      break;
   case ACIOC_WRITE_Q_CONST:
      if (((struct queue_const *) (op_parmp->v))->wait_time >
         ((struct queue_const *) (op_parmp->v))->hog_time)
         return(ENXIO);

      if (((struct queue_const *) (op_parmp->v))->wait_time < 0)
         return(ENXIO);

      if (major(op_parmp->dev) == AUTOX0_MAJ_CHAR) {
         surface = 0;
      } else {
         surface = ac_surface(op_parmp->dev);
      }

      if (surface == 0) {
         for (i = 0; i < MAX_SUR; i++) {
            ac_device_ptr->ac_wait_time[i] = ((struct queue_const *) (op_parmp->v))->wait_time;
            ac_device_ptr->ac_hog_time[i]  = ((struct queue_const *) (op_parmp->v))->hog_time;
         }
      }
      else {
         ac_device_ptr->ac_wait_time[surface] = ((struct queue_const *) (op_parmp->v))->wait_time;
         ac_device_ptr->ac_hog_time[surface] = ((struct queue_const *) (op_parmp->v))->hog_time;
      }
      break;
   case ACIOC_INITIALIZE_ELEMENT_STATUS: 
      /* must force the xport daemon to flush all the close delays */

      ac_device_ptr->transport_busy = 1;
      xport_reqp = init_xport_request();
      /* no move parameters necessary */
      xport_reqp->pid = 0;
      xport_reqp->x_flags |=  X_FLUSH_CLOSE;
      /* keep other requests out while deamon starts up */
      if (AC_DEBUG & QUEUES) {
         msg_printf0("XPORT");
         print_xport_queue(ac_device_ptr->xport_queue_head);
      }
      ADD_TO_LIST(ac_device_ptr->xport_queue_head,xport_reqp);
      if (AC_DEBUG & QUEUES) 
         print_xport_queue(ac_device_ptr->xport_queue_head);
      pri = spl6();
      wakeup(&(ac_device_ptr->xport_queue_head));
      sleep(xport_reqp,PRIBIO);
      (void) splx(pri);
      FREE_ENTRY(xport_reqp,xport_request);
      error = do_ioctl_initialize_element_status(ac_device_ptr);
      break;
   case ACIOC_READ_ELEMENT_STATUS:
      /* must force the xport daemon to flush all the close delays */

      ac_device_ptr->transport_busy = 1;
      xport_reqp = init_xport_request();
      /* no move parameters necessary */
      xport_reqp->pid = 0;
      xport_reqp->x_flags |=  X_FLUSH_CLOSE;
      /* keep other requests out while deamon starts up */
      if (AC_DEBUG & QUEUES) {
         msg_printf0("XPORT");
         print_xport_queue(ac_device_ptr->xport_queue_head);
      }
      ADD_TO_LIST(ac_device_ptr->xport_queue_head,xport_reqp);
      if (AC_DEBUG & QUEUES) 
         print_xport_queue(ac_device_ptr->xport_queue_head);
      pri = spl6();
      wakeup(&(ac_device_ptr->xport_queue_head));
      sleep(xport_reqp,PRIBIO);
      (void) splx(pri);
      FREE_ENTRY(xport_reqp,xport_request);
      ac_device_ptr->read_element_status_flag = 1;
      break;
   case ACIOC_READ_Q_STATS: 
      /* prevent interupts so that value of head pointer stays same*/
      /* pri = spl6(); */
      get_q_stats (op_parmp->v,ac_device_ptr);
      /* splx(pri); */
      break;
   default:
      return(ENOTTY);
   }
   return (error);
}

/*
 * Call the particular synchronous operation.  
 * For the 800, the logical unit number
 * is passed in rather than the scsi_address.
 */
int
call_sync_op(op,op_parmp,scsi_addr)
   int (*op) ();
   struct operation_parms *op_parmp;
   int scsi_addr;
{
   int error = 0;

#ifndef _WSIO
   dev_t addr_dev = makedev(0,makeminor(0,scsi_addr,0,0));
#else
   dev_t addr_dev = makedev(0,ac_minor(0,scsi_addr,0,0));
#endif

   /* nested if's since a switch statement requires compile 
   * time constants
   */
#ifdef KERNEL_DEBUG_ONLY
   if (force_error.call_sync_op) error = EIO;
   else
#endif KERNEL_DEBUG_ONLY
   if (major(op_parmp->dev) == drive_maj_blk) {
      /* look for block entry points */
      if (op == close_surface) {
         op(op_parmp->ac_device_ptr,op_parmp->drive_ptr);
#ifdef _WSIO
      } else if (op == autoch_disk_capacity) {
         op_parmp->dev |= addr_dev;
         error = op(op_parmp->dev, op_parmp->v, op_parmp->drive_ptr);
      } else if (op == autoch_disk_describe) {
         op_parmp->dev |= addr_dev;
         /* added op_parmp->code to the parameters passed to
          * autoch_disk_describe(), to address the binary compatibility 
          * problem if someone calls this with an old binary.  This
          * allows us to return the correct size, without overwriting
          * their memory. 
          */
         error = op(op_parmp->dev, op_parmp->v, op_parmp->drive_ptr,
                    op_parmp->code);
#endif /* _WSIO */
      } else if (op == ac_noop) {
       /* don't really need to do anything */
           if (AC_DEBUG & DEADLOCK)
              msg_printf1("performing noop on drive %x\n",op_parmp->drive_ptr);
      } else {
         printf("op = 0x%x\n",op);
         panic("illegal operation in call_sync_op:1");
         }
      }
   else { /* look for char entry points */
      if (op == close_surface) {
         op(op_parmp->ac_device_ptr,op_parmp->drive_ptr);
      } else if (op == cdevsw[major(op_parmp->dev)].d_read) {
         op_parmp->dev |= addr_dev;
         error = op(op_parmp->dev,op_parmp->uio);
      } else if (op == cdevsw[major(op_parmp->dev)].d_write)  {
         op_parmp->dev |= addr_dev;
         error = op(op_parmp->dev,op_parmp->uio);
      } else if (op == autoch_format)  {
         op_parmp->dev |= addr_dev;
         error = op(op_parmp->dev,op_parmp->v,op_parmp->drive_ptr);
      } else if (op == autoch_disk_capacity)  {
         op_parmp->dev |= addr_dev;
         error = op(op_parmp->dev, op_parmp->v, op_parmp->drive_ptr);
      } else if (op == autoch_disk_describe)  {
         op_parmp->dev |= addr_dev;
         /* added op_parmp->code to the parameters passed to
          * autoch_disk_describe(), to address the binary compatibility 
          * problem if someone calls this with an old binary.  This
          * allows us to return the correct size, without overwriting
          * their memory. 
          */
         error = op(op_parmp->dev, op_parmp->v, op_parmp->drive_ptr,
                    op_parmp->code);
#ifdef PASS_THRU
      } else if (op == autoch_sioc_io)  {
         op_parmp->dev |= addr_dev;
         error = op(op_parmp->dev, op_parmp->v, op_parmp->drive_ptr);
#endif
      } else if (op == autoch_sioc_erase)  {
         op_parmp->dev |= addr_dev;
         error = op(op_parmp->dev, op_parmp->v, op_parmp->drive_ptr);
      } else if (op == autoch_set_wwv_mode)  {
         op_parmp->dev |= addr_dev;
         error = op(op_parmp->dev, op_parmp->v, op_parmp->drive_ptr);
      } else if (op == autoch_set_woe_mode)  {
         op_parmp->dev |= addr_dev;
         error = op(op_parmp->dev, op_parmp->v, op_parmp->drive_ptr);
      } else if (op == autoch_sioc_verify) {
         op_parmp->dev |= addr_dev;
         error = op(op_parmp->dev, op_parmp->v, op_parmp->drive_ptr);
      } else if (op == autoch_sioc_verify_blank)  {
         op_parmp->dev |= addr_dev;
         error = op(op_parmp->dev, op_parmp->v, op_parmp->drive_ptr);
      } else if (op == autoch_sioc_set_ir)  {
         op_parmp->dev |= addr_dev;
         error = op(op_parmp->dev, op_parmp->v, op_parmp->drive_ptr);
#ifndef _WSIO
      } else if (op == autoch_sioc_sync_cache)  {
         op_parmp->dev |= addr_dev;
         error = op(op_parmp->dev, op_parmp->v, op_parmp->drive_ptr);
#endif /* ! _WSIO */
      } else if (op == autoch_sioc_xsense)  {
         op_parmp->dev |= addr_dev;
         error = op(op_parmp->dev, op_parmp->v, op_parmp->drive_ptr);
      } else if (op == autoch_sioc_inquiry)  {
         op_parmp->dev |= addr_dev;
         error = op(op_parmp->dev, op_parmp->v, op_parmp->drive_ptr);
#ifdef _WSIO
      } else if (op == autoch_sioc_capacity)  {
         op_parmp->dev |= addr_dev;
         error = op(op_parmp->dev, op_parmp->v, op_parmp->drive_ptr);
#endif /* _WSIO */
      } else if (op == ac_noop) {
       /* don't really need to do anything */
           if (AC_DEBUG & DEADLOCK)
              msg_printf1("performing noop on drive %x\n",op_parmp->drive_ptr);
      } else {
         printf("op = 0x%x\n",op);
         panic("illegal operation in call_sync_op:2");
      }
   }
   return(error);
}


/*
 * Choose method to use to wakeup processes and call it.
 */
void
prepare_for_next_request(oldsreqp)
   struct surface_request *oldsreqp;
{
   struct surface_request *sreqp=NULL;
   int sync_match=0,async_match = 0;
   struct ac_drive *drive_ptr = oldsreqp->drive_ptr;
   struct ac_device *ac_device_ptr = oldsreqp->ac_device_ptr;
   short pid = oldsreqp->pid;

   /* decrement reference count */
   (drive_ptr->ref_count)--;

   DELETE_FROM_LIST(oldsreqp->ac_device_ptr->sreqq_head,oldsreqp);

   if (drive_ptr->ref_count == 0) {
      /* the last busy request for this drive */
      if ((hog_time_ok(drive_ptr) || ac_device_ptr->transport_busy) &&
         !(ac_device_ptr->jammed) &&
         (drive_ptr->surface != EMPTY)) {
         /* schedule all async requests for the current surface */
         drive_ptr->drive_state = DO_OP;
         start_async_requests(drive_ptr,drive_ptr->surface);
         /* can process other requests for this drive */
         if (same_sur_in_q(drive_ptr->surface,ac_device_ptr)) {
            /* start up all requests for the same surface as inserted */
            if (AC_DEBUG & BOT_CASE) {
               msg_printf1("%d bottom switch\n", pid);
               msg_printf2("%d    same_sur_in_q surface = %x\n",pid, oldsreqp->surface);
            }
            /* loop through all requests looking for same surface */
            FOR_EACH_SWAP_REQUEST
               if (AC_DEBUG&BOT_CASE)
                  msg_printf2("before if sreqp is %x,sreqp->surface is %x\n", sreqp,sreqp->surface);
               if (sreqp->surface == drive_ptr->surface) {
                  if (!(sreqp->r_flags & R_ASYNC)) {
                     sync_match = 1;
                     /* only start sync requests */
                     if (AC_DEBUG&BOT_CASE)
                        msg_printf1("calling startup_req sreqp is %x\n", sreqp);
                     startup_request(sreqp,NULL);
                  }
                  else {
                     async_match = 1;
                     /* should the op be scheduled here? */
                  }
               }
            END_FOR
            if ((!sync_match) && (async_match)) {
               if (AC_DEBUG & BOT_CASE) {
                  msg_printf1("%d bottom switch, async\n", pid);
                  msg_printf2("%d    same_sur_NOT_in_q surface = %x\n",pid, oldsreqp->surface);
               }
               if (drive_ptr->interrupt_request == NULL)
                drive_ptr->interrupt_request = init_xport_request();
               wait_for_wait_time(drive_ptr);
            }
            
            if ((!async_match) && (!sync_match))
               panic("didn't find match");
         } else {
            if (AC_DEBUG & BOT_CASE) {
               msg_printf1("%d bottom switch\n", pid);
               msg_printf2("%d    same_sur_NOT_in_q surface = %x\n",pid, oldsreqp->surface);
            }
            if (drive_ptr->interrupt_request == NULL)
               drive_ptr->interrupt_request = init_xport_request();
            wait_for_wait_time(drive_ptr);
         }
      } else {
         /* hog time has expired */
         /* or jammed */
         /* or surface == EMPTY */
         if (AC_DEBUG & BOT_CASE) {
            msg_printf1("%d bottom switch\n", pid);
            msg_printf2("%d    hog_time_expired surface = %x\n",pid, oldsreqp->surface);
         }
         drive_ptr->drive_state = NOT_BUSY;
         sreqp = get_next_request (drive_ptr,drive_ptr->ac_device_ptr);
         startup_request(sreqp,NULL);
      }
   } else {
      /* not the last busy request for this drive */
      if (AC_DEBUG & BOT_CASE) {
         msg_printf1("%d bottom switch\n", pid);
         msg_printf2("%d    ref_count!=0 surface = %x\n",pid, oldsreqp->surface);
      }
   }
   FREE_ENTRY(oldsreqp,surface_request);
}


/*
 * This procedure loads up the surface request and adds it to the surface request
 * queue.
 */
struct surface_request *
add_request (ac_device_ptr, op, op_parmp, surface)
   struct ac_device *ac_device_ptr;
   int (*op) ();
   struct operation_parms *op_parmp;
   int surface;
{
   struct surface_request *sreqp;

   init_sreq (&sreqp);
   sreqp->surface = surface;
   sreqp->pid = u.u_procp->p_pid;
   sreqp->ac_device_ptr = ac_device_ptr;
   sreqp->op = op;
   sreqp->op_parmp = op_parmp;
   if (op == bdevsw[drive_maj_blk].d_strategy) {
      /* printf("adding async request pid=%d\n",sreqp->pid); */
      sreqp->r_flags |= R_ASYNC;
      sreqp->bp = op_parmp->bp;
   }
   if (AC_DEBUG & QUEUES) 
      print_surface_queue(ac_device_ptr);
   ADD_TO_LIST(ac_device_ptr->sreqq_head,sreqp);
   if (AC_DEBUG & QUEUES) 
      print_surface_queue(ac_device_ptr);
   return (sreqp);
}


/*
 * Get drive and mark it busy.
 */
void
allocate_drive (sreqp)
   struct surface_request *sreqp;
{
   get_drive(sreqp->ac_device_ptr, sreqp->surface, &(sreqp->drive_ptr));

   /* increment reference count */
   (sreqp->drive_ptr->ref_count)++;

   /* mark request busy */
   sreqp->r_flags |= R_BUSY;

   sreqp->drive_ptr->surface_inproc = sreqp->surface;

   /* mark drive allocated if a swap will be required and not busy */
   /* this is so that another process won't grab it */
   if (    (sreqp->drive_ptr->drive_state == NOT_BUSY) &&
         (sreqp->surface != sreqp->drive_ptr->surface) ) {
      sreqp->drive_ptr->drive_state = ALLOCATED;
   }
}

/*
 * The xport daemon deadlocked.  This routine is called to 
 * start up the recovery sequence.
 *
 */

int
initiate_deadlock_recovery(dlp)
   struct ac_deadlock_struct *dlp;
{
   struct ac_device *ac_device_ptr;
   struct xport_request *xport_reqp;
   int pri;
   
   pri = spl6();
   if (!(dlp->flags & DL_TIMER_ON)) {
      /* msg_printf1("INITIATE_DEADLOCK_RECOVERY FIRED IMPROPERLY 0x%x\n",
                   dlp->flags); */
      /* printf("INITIATE_DEADLOCK_RECOVERY FIRED IMPROPERLY\n"); */
   } else {
      dlp->flags &= ~DL_TIMER_ON;
      dlp->flags |= DEADLOCKED;
      ac_device_ptr = (struct ac_device *)dlp->dl_data;
      xport_reqp = ac_device_ptr->recover_reqp;
      xport_reqp->x_flags = X_RECOVER;
      ADD_TO_LIST(ac_device_ptr->spinup_queue_head,xport_reqp);
      if (AC_DEBUG & DEADLOCK) 
         msg_printf1("waking up recovery request for spinup queue %x\n",xport_reqp); 
      wakeup(&(ac_device_ptr->spinup_queue_head));
   }
   splx(pri);

}



/*
 * autochanger previously jammed, so try to reset our important data structures
 * to allow continued operation (in case the user has repaired)
 */
int
reset_data_structures(ac_device_ptr)
    struct ac_device *ac_device_ptr;
{
    int page_code, i,j;
    int error=0;
    struct buf *el_bp;
    struct ac_drive *temp_drive_ptr;
    extern struct buf *geteblk();
    unsigned char    *drive_info;
#ifdef __hp9000s700
    dev_t dev;
#endif /* __hp9000s700 */

    if (AC_DEBUG & RESET_DS) 
        msg_printf0("Entering Reset data structures\n");
    /* set transport busy */
    ac_device_ptr->transport_busy = 1;    /* picker in use */
    /* should force the drives close here so they won't have prevent media set */
        for (i = 0; i < NUM_DRIVE; i++) {
#ifdef __hp9000s700
        dev = makedev(drive_maj_blk,
                      ac_minor(ac_card(ac_device_ptr->device),
                               ac_device_ptr->ac_drive_table[i].scsi_address,
                               0, 0));

        if (m_scsi_bus(dev) != NULL &&
            m_scsi_tgt(dev) != NULL &&
            m_scsi_lun(dev) != NULL)
#else
        if (ac_device_ptr->ac_drive_table[i].surface != EMPTY)
#endif /* __hp9000s700 */
        {
                  temp_drive_ptr = &(ac_device_ptr->ac_drive_table[i]);
            if (AC_DEBUG & RESET_DS) 
                msg_printf1("Closing drive %x\n",temp_drive_ptr);
#ifndef _WSIO
            /* close all the sections */
            for (j = 0; j <= MAX_SECTION ; j++) {
                (*cdevsw[drive_maj_char].d_close)(makedev(drive_maj_char,
                makeminor(0,temp_drive_ptr->logical_unit,0,j)));
            }
#else
                  (*cdevsw[drive_maj_char].d_close)(makedev(drive_maj_char,
                     ac_minor(ac_card(ac_device_ptr->device),
                              temp_drive_ptr->scsi_address,0,0)),0);
#endif
        }
        }
    if (AC_DEBUG & RESET_DS) 
        msg_printf0("About to init el\n");
    error = autox0_ioctl(ac_device_ptr->device,AUTOX0_INITIALIZE_ELEMENT_STATUS,0);
    if (AC_DEBUG & RESET_DS) 
        msg_printf0("Finished init el\n");
    if (error) {
        ac_device_ptr->jammed = 1;
        printf("AC: initialize_element_status failed\n");
        if (AC_DEBUG&NO_PANICS)
           msg_printf0("AC: initialize_element_status failed\n");
        return(EIO);    /* still jammed! */
    }

/*
 * get element status atomically, to avoid syncronization problems
 * which can occur when the autochanger is in motion
 * -paulc 10/26/1989
 */
    el_bp        = geteblk(EL_STATUS_SZ);
    el_bp->b_dev    = ac_device_ptr->device;
    page_code    = ALL_PC;
    ac_read_element_status(ac_device_ptr,el_bp,page_code);
    if (error = geterror(el_bp)) {
        ac_device_ptr->jammed = 1;
        msg_printf0("AC: read_element_status failed\n");
        msg_printf("AC: read_element_status failed\n");
        if (AC_DEBUG&NO_PANICS)
            printf("AC: read_element_status failed\n");
        brelse(el_bp);
        return(EIO);    /* still jammed! */
    }

    /* some elements of this structure must be zeroed */
    for (i = 0; i < NUM_DRIVE; i++) {
        ac_device_ptr->ac_drive_table[i].surface = EMPTY;
        ac_device_ptr->ac_drive_table[i].surface_inproc = EMPTY;
        ac_device_ptr->ac_drive_table[i].drive_state = NOT_BUSY;
    }

    drive_info = find_status_page((unsigned char *)el_bp->b_un.b_addr,DATA_TRANSFER_PC);
    error = set_drive_info(ac_device_ptr,drive_info);
    brelse(el_bp);
    if (error) {
        ac_device_ptr->jammed = 1;
        msg_printf("AC: set_drive_info failed\n");
        msg_printf0("AC: set_drive_info failed\n");
        if (AC_DEBUG&NO_PANICS)
            printf("AC: set_drive_info failed\n");
        return(EIO);    /* still jammed! */
    }
    for (i = 0; i < NUM_DRIVE; i++) {
       if (ac_device_ptr->ac_drive_table[i].surface != EMPTY) {
          temp_drive_ptr = &(ac_device_ptr->ac_drive_table[i]);
          error = prepare_surface_for_use(temp_drive_ptr,
             ac_device_ptr->ac_drive_table[i].surface,ac_device_ptr->xport_bp,NOT_BUSY);
          if (error) {
             ac_device_ptr->jammed = 1;
             msg_printf("AC: prepare_surface_for_use failed\n");
             msg_printf0("AC: prepare_surface_for_use failed\n");
             if (AC_DEBUG&NO_PANICS)
                 printf("AC: prepare_surface_for_use failed\n");
             return(EIO);    /* still jammed! */
          }
       }
    }
    ac_device_ptr->jammed = 0;
    /* start the requests waiting for the transport*/
    if (AC_DEBUG & RESET_DS) 
        msg_printf0("Exiting Reset data structures\n");
    return(0);
}

/*
 * This procedure takes a driver operation (close, read, 
 * write and strategy) and processes it.
 * In other words it does the work to get the appropriate side of media in 
 * the drive and executes the operation.
 * Note:  This routine is not called for open. It is not needed. 
 * MP Note: For HP-UX 9.0, if the autochanger becomes MP, then it must
 *      take care to invoke UP Emulation (if necessary) for those functions
 *      in "op" which are from UP Drivers.
 */ 
int
process_op (ac_device_ptr, op, op_parmp, surface, print_stringp)
   struct ac_device *ac_device_ptr;
   int (*op) ();
   struct operation_parms *op_parmp;
   int surface;
   char *print_stringp;
{
   int error = 0, pri;
   int error_switch;
   int escapecode = 0;
   int had_to_untimeout = 0;
   struct surface_request *sreqp;
   struct xport_request *xport_reqp;
   struct ac_drive *drive_ptr;
   struct ac_deadlock_struct *dlp;
   extern void allocate_drive();

   /* critical section to protect from wait_time_done */
   pri = spl6(); 

   if (AC_DEBUG & OPERATION) {
      msg_printf2("\n%d:%s\n", u.u_procp->p_pid, print_stringp);
   }

   sreqp = add_request(ac_device_ptr, op, op_parmp, surface);
try {
   switch (calculate_scheduling_method(sreqp)) {
      case AC_NO_DELAY:
            sreqp->result = AC_NO_DELAY;
            allocate_drive (sreqp);
            break;
      case AC_DELAY:
            sreqp->result = AC_DELAY;
            if (sreqp->r_flags & R_ASYNC) {
               splx(pri);
               return(0); /* async requests must return immediately */
            }
            else {
               if (sleep(sreqp,PWAIT | PCATCH)) {
                  /* check for wait timer expiring during an interrupt of */
                  /*   a request. */
                  if (!(sreqp->r_flags & R_BUSY)) {
                error_switch = 0;
             escape(EINTR);
                  }
               }
            }
            break;
      case AC_FLUSH:
            sreqp->result = AC_FLUSH;
            had_to_untimeout = 0;
            if (ac_device_ptr->pmount_lock & IN_PUNMOUNT) {
               dlp = &(ac_device_ptr->deadlock);
               if (dlp->flags & DL_TIMER_ON) {
                  untimeout(dlp->recover_func,(caddr_t)dlp); 
                  had_to_untimeout = 1;
                  dlp->flags &= ~DL_TIMER_ON;
               }
               /* msg_printf("process_op untimeout\n"); */
            }
            /* print_surface_queue(ac_device_ptr); */
            if (sreqp->r_flags & R_ASYNC) {
               current_sur_inserted(sreqp->ac_device_ptr,
                  sreqp->surface,&drive_ptr);
               sreqp->drive_ptr = drive_ptr;
               /* increment reference count */
               (drive_ptr->ref_count)++;
            
               /* mark request busy */
               sreqp->r_flags |= R_BUSY;
            
               /* for same surface and async request */
               if (AC_DEBUG & PMOUNT) {
                  msg_printf1("process_op flush: starting request for pid %d\n",sreqp->pid);
                  msg_printf1("Calling scsi strategy with bp = %x\n",sreqp->bp);
               }
#ifndef _WSIO
               call_async_op(sreqp,drive_ptr->logical_unit);
#else /* s300 and s700 */
               call_async_op(sreqp,drive_ptr->scsi_address);
#endif
               DELETE_FROM_LIST(ac_device_ptr->sreqq_head,sreqp);
               FREE_ENTRY(sreqp,surface_request);

               if (AC_DEBUG & PMOUNT) 
                  msg_printf0("Calling start async during flush\n"); 

               /* Make sure any waiting requests get processed during flushing */
               start_async_requests(drive_ptr,surface);

               if (AC_DEBUG & PMOUNT) 
                  msg_printf0("finished start async during flush\n"); 

               /* decrement reference count */
               (drive_ptr->ref_count)--;
            }
            else    /* should only get async requests. */
               panic("sync op in AC_FLUSH in process_op");
            
            if (ac_device_ptr->pmount_lock & IN_PUNMOUNT) {
               if (dlp->flags & DL_TIMER_ON)
                  panic("Process_op:  Timer started when already running");
               else {
                  if (had_to_untimeout) {
                     timeout(dlp->recover_func,(caddr_t)dlp,HZ*DEADLOCK_TIMEOUT); 
                     dlp->flags |= DL_TIMER_ON;
                     had_to_untimeout = 0;
                  }
               }
               /*msg_printf("process_op   timeout\n");*/
            }
            splx(pri); 
            return(0);  /* just do the function and return */
            break;
            
      default:
            panic ("bad return value from calculate_scheduling_method in ac driver");
   }

   if (ac_device_ptr->jammed) {
      /* autochanger jammed while you were asleep */
      /* we use this arbitrary drive_ptr because drive_ptr info is  */
      /* not valid at this point. Assume no other processes running*/
      sreqp->drive_ptr = &(sreqp->ac_device_ptr->ac_drive_table[0]);
      /* increment reference count */
      (sreqp->drive_ptr->ref_count)++;
      error = ac_send_reset(ac_device_ptr);
      if (error) {
         ac_device_ptr->jammed = 1;
         ac_device_ptr->transport_busy = 0;
         error_switch = 1;
         escape(error);
      }
      /* decrement reference count */
      (sreqp->drive_ptr->ref_count)--;
      sreqp->drive_ptr = NULL;
      allocate_drive (sreqp);
   }

   if(sreqp->surface != sreqp->drive_ptr->surface) { /* swap required*/
      xport_reqp = send_xport_request(ac_device_ptr,sreqp,NULL);
      if (AC_DEBUG & XPORT_OP) {
         msg_printf0("process_op: waking up xport daemon ");
         msg_printf1("for sreqp = %x\n",sreqp);
         msg_printf1("surface = %d ",sreqp->surface);
         msg_printf1("drive = 0x%x ",sreqp->drive_ptr);
         msg_printf1("xportp = 0x%x\n",xport_reqp);
      }
      if(sreqp->r_flags & R_ASYNC) {
         splx(pri);
         return(0);  /* async requests must return before I/O is performed */
         }
      else
         if(sleep(xport_reqp,PRIBIO|PCATCH)) {
            error_switch = 3;
            escape(EINTR);
         }
      if (AC_DEBUG & XPORT_OP)
         msg_printf0("process_op: returned from xport daemon\n"); 
      if (!(xport_reqp->x_flags & X_DONE)) { /* move had an error */ 
         error = xport_reqp->error;
         printf("Move error in process op: error = %d\n",error);
         if(!error) 
            panic("ioctl: Failed to complete but gave no error");
         /* panic("Move failed in process op"); */
            error_switch = 4;
            escape(error);
  
      }
      FREE_ENTRY(xport_reqp,xport_request);
      }
   if (!error) {
      if (AC_DEBUG&AC_STATES)
         msg_printf1("%d process_op: do_op\n",sreqp->pid);
      sreqp->drive_ptr->drive_state = DO_OP;

      if (sreqp->r_flags & R_ASYNC) {
#ifndef _WSIO
         call_async_op(sreqp,sreqp->drive_ptr->logical_unit);
#else /* s300 and s700 */
         call_async_op(sreqp,sreqp->drive_ptr->scsi_address);
#endif
      }
      else {
         /* loading these up special for close_surface() called */
         /* from call_sync_op() */
         /* close_surface is the op() set on autoch_close, */
         sreqp->op_parmp->ac_device_ptr = ac_device_ptr;
         sreqp->op_parmp->drive_ptr = sreqp->drive_ptr;
#ifndef _WSIO
         if(error=call_sync_op(sreqp->op,sreqp->op_parmp,sreqp->drive_ptr->logical_unit)) {
#else /* s300 and s700 */
         if(error=call_sync_op(sreqp->op,sreqp->op_parmp,sreqp->drive_ptr->scsi_address)) {
#endif
            error_switch = 5;
            escape(error);
         }
      }
   }
   prepare_for_next_request(sreqp);

   /* restore interrupts */
   splx(pri);

   return (0);

} 
recover: {
   if (AC_DEBUG&PANICS)
      panic("error in process_op");
   switch (error_switch) {
      /*it is intended that each item fall thru to the next*/
      /*this will undo everything in reverse order*/
      case 5: prepare_for_next_request(sreqp);
         break;
      case 4: /* had a move fail */
              prepare_for_next_request(sreqp);
              break;
      case 3: /* a process sleeping on the xport daemon was killed */
         xport_reqp->x_flags |= X_NO_WAITING;
         msg_printf1("%d process_op: killed op\n",sreqp->pid);
         /* Must now wait for the autochanger to get to a 
            known point in the move process */
         sleep(xport_reqp,PRIBIO);
         FREE_ENTRY(xport_reqp,xport_request);
      case 2:
      case 1: prepare_for_next_request(sreqp);
         break;
      case 0:
         /* A sleeping request was being killed */
         DELETE_FROM_LIST(ac_device_ptr->sreqq_head,sreqp);
         FREE_ENTRY(sreqp,surface_request);
         break; 
      default:
         panic("Bad error recovery in process_op");
   }

   /* restore interrupts */
   splx(pri);

   return (escapecode);
   }
}





