/*
 * @(#)dgram_aud.h: $Revision: 1.4.83.4 $ $Date: 93/09/17 18:24:57 $
 * $Locker:  $
 */

#ifndef _SYS_DGRAM_AUDIT_INCLUDED /* allows multiple inclusions */
#define _SYS_DGRAM_AUDIT_INCLUDED


/* BEGIN_IMS DGRAM_AUDIT *
 ********************************************************************
 ****
 ****	AUDITDGRAM
 ****
 ********************************************************************
 * Input Parameters
 *	error		status of call being audited (0 = no error)
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	TRUE		the audit call should be made
 *	FALSE		the audit call should be omitted
 *
 * Globals Referenced
 *	audit_state_flag
 *	user structure: u_audproc and u_audsusp fields
 *	aud_event_table
 *
 * Description
 *	AUDITDGRAM determines whether it is appropriate to make
 *	an audit of a network kernel event
 *
 * Algorithm
 *	AUDITDGRAM is implemented as a complex conditional
 *	clause.  It checks first whether auditing is turned on for
 *	the entire system, then  whether auditing is turned on for
 *	this particular user process, and whether auditing has
 *	been temporarily suspended for this user, finally whether
 *	for this type of event, success is being audited and no
 *	error has occurred or else failure is being audited and
 *	and error has arisen.
 * 
 * To Do List
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	none
 ********************************************************************
 * END_IMS AUDITDGRAM
 */
extern struct aud_event_tbl aud_event_table;


#define	AUDITDGRAM(error) ((AUDITON ()) && \
(((aud_event_table.ipcdgram & PASS) && (! error)) \
|| ((aud_event_table.ipcdgram & FAIL) && (error))))

#endif /* _SYS_DGRAM_AUDIT_INCLUDED */
