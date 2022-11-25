/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/dgram_aud.c,v $
 * $Revision: 1.6.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:04:12 $
 */
/* BEGIN_IMS DGRAM_AUDIT *
 ********************************************************************
 ****
 ****	AUDIT DATAGRAMS
 ****
 ********************************************************************
 * Input Parameters
 *	error (usually u.u_error)
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	audit_state_flag
 *	user structure: u_audproc and u_audsusp fields
 *	aud_event_table
 *	
 *
 * Description
 *	
 *
 * Algorithm
 *	We build an auditing record, just like in aud_write.
 *	Into it  goes an indication of "send" or "recieve" as
 *	well as the adress the datagram is sent to or recieved from
 *
 *
 *
 *
 *
 * 
 * To Do List
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	kern_aud_wr
 ********************************************************************
 * END_IMS AUDITDGRAMS
 */

#ifdef AUDIT
#include "../h/audit.h"
#include "../h/types.h"
#include "../h/ipc.h"
#include "../h/sem.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/errno.h"

#include "../h/param.h"
#include "../h/socket.h"
#include "../netinet/in.h"
#include "../net/route.h"
#include "../netinet/in_pcb.h"


#define TO 1 
#define FROM 0

  


  

void
audit_recv_dgram ( sin_port, sin_addr ,error)
     u_short sin_port;
     struct in_addr * sin_addr;
     int error;
{
  
  dgram_audit ( FROM, sin_port,  sin_addr, error );

}


void
audit_send_dgram ( msg_name, msg_len, error )
     caddr_t msg_name;
     int msg_len;
     int error;
     
{
  struct sockaddr_in * ptr;

  if (msg_len == 0) 
      return;

  
  
  ptr= (struct sockaddr_in * ) msg_name;
  

     dgram_audit ( TO, ptr->sin_port ,  ptr->sin_addr , error);

	
}



  
static  int
dgram_audit ( to_from, port, addr, error)
     int to_from;
     ushort port;
     struct in_addr  addr;
     int error;
     
{
  int original_u_error, i;
  register struct proc *p;
  char  message [40] ,*ptr ;
  struct audit_hdr aud_head;
  struct audit_arg args[MAX_AUD_ARGS];


  ptr = (char *)&addr;
  
#define UC(b)   (((int)b)&0xff)
  if (to_from == TO)
    sprintf(message,40, "To  : %d.%d.%d.%d, port: %d ", UC(ptr[0]), UC(ptr[1]),
	  UC(ptr[2]), UC(ptr[3]), port);
  else  
    sprintf(message,40, "From: %d.%d.%d.%d, port: %d ", UC(ptr[0]), UC(ptr[1]),
	  UC(ptr[2]), UC(ptr[3]), port);
  

  /* save the u_error value, because it may not be preserved across the
   * kern_aud_wr call
   */
  
  original_u_error = u.u_error;
  aud_head.ah_error= error;
  aud_head.ah_event= EN_IPCDGRAM;
  
  
     

  p = u.u_procp;
  /* Fill in pid and time for header */
  aud_head.ah_pid = p->p_pid;
  aud_head.ah_time = time.tv_sec; /* accurate to the second */
  aud_head.ah_len = strlen(message);
  
   /* Write the record */
   args[0].data = (caddr_t)&(aud_head);
   args[0].len = sizeof(aud_head);
   args[1].data = (caddr_t) message;
   args[1].len =  strlen (message);
   for (i=2; i < MAX_AUD_ARGS; i++) {
      args[i].data = NULL;
      args[i].len = 0;
   }
   u.u_error = 0;
   if (u.u_error = kern_aud_wr(args))
      return(-1);

   return(0);
}

#endif /* AUDIT */
