/* $Log:	audnetd.c,v $
 * Revision 70.1  92/01/29  17:11:26  17:11:26  ssa
 * Author: abraham@hpisoe4.cup.hp.com
 * telnetd uses event type NA_EVNT_OTHER which was not understood by 
 * audit_daemon.  Modified so that all other events besides EN_IPCCLOSE and
 * EN_PASSWD are logged under EN_IPCOPEN
 * 
 * Revision 64.3  89/02/21  15:54:17  15:54:17  human
 * Audit state changed to audit event,
 * password change event added, new
 * services rexec and named added,
 * u_short changed to uid_t where
 * appropriate.
 * 
 * Revision 64.2  89/02/12  19:41:47  19:41:47  rsh (R. Scott Holbrook[Ft. Collins])
 * Replaced sprintf() with strcpy(), strcat() and ultoa() so we don't need doprnt
 * Removed function dot_format, using inet_ntoa() instead.
 * General cleanup to reduce the size of the object file (common string
 * re-usage, etc).
 * Added stuff for name space pollution cleanup.
 * 
 * Revision 64.1  89/02/08  15:36:37  15:36:37  lkc (Lee Casuto)
 * Initial checkin being done for Aaron since I'm such a nice guy
 * 
 * Revision 1.6  89/02/07  16:51:00  16:51:00  human (Aaron Schuman)
 * 
 */
/*
 * audit_daemon() is a libc library function providing uniform
 * access to the audit subsystem by all networking daemons.
 */

#ifdef _NAMESPACE_CLEAN
#define audwrite _audwrite
#define strcpy _strcpy
#define strcat _strcat
#define strlen _strlen
#define strncat _strncat
#define inet_ntoa _inet_ntoa
#define ultoa _ultoa
#endif /* _NAMESPACE_CLEAN */

#include <sys/audit.h>
#include <audnetd.h>
#include <string.h>
#include <stdio.h>

extern char *inet_ntoa();
extern char *ultoa();

#define	LINE_SZ	256	/* size of a string buffer	*/
#define	SERV_SZ	64	/* size of service name string	*/

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _audit_daemon audit_daemon
#define audit_daemon _audit_daemon
#endif /* _NAMESPACE_CLEAN */

void
audit_daemon
	(service,	/* select an NA_SERV_ constant	*/
	result,		/* select an NA_RSLT_ constant	*/
	valid_mech,	/* select an NA_VALI_ constant	*/
	remote_system,	/* four byte network address	*/
	remote_user,	/* uid of user at remote system	*/
	local_system,	/* four byte network address	*/
	local_user,	/* local euid (NOT daemon uid)	*/
	event,		/* select an NA_EVNT_ constant	*/
	ignore_mask,	/* which parms for this func	*/
	 		/* are invalid and should be	*/
			/* ignored	*/
	free_field)	/* null terminated string	*/

char *	service;
u_short	result;
u_short	valid_mech;
u_long	remote_system;
uid_t	remote_user;
u_long	local_system;
uid_t	local_user;
u_short	event;
u_short	ignore_mask;
char *	free_field;

{
static char unspecified[] = "unspecified\n";

struct self_audit_rec	audrec;
#define	body	audrec.aud_body.text

char	line		[LINE_SZ];

static	struct rslt {
	int	value;
	char *	name;
} rslt [] = {
	NA_RSLT_SUCCESS, "success",
	NA_RSLT_FAILURE, "failure",
	};
#define	NUM_RSLT ((sizeof(rslt)/sizeof(struct rslt)))

static	struct vali {
	int	value;
	char *	name;
} vali [] = {
	NA_VALI_OTHER,	"other   ",
	NA_VALI_PASSWD,	"passwd  ",
	NA_VALI_RUSEROK,"ruserok ",
	NA_VALI_UID,	"uid     ",
	};
#define	NUM_VALI ((sizeof(vali)/sizeof(struct vali)))

static  struct evnt {
	int     value;
	char *  name;
} evnt [] = {
	NA_EVNT_OTHER,  "other           ",
	NA_EVNT_START,  "start of service",
	NA_EVNT_STOP ,  "stop  of service",
	NA_EVNT_PASSWD, "password change ",
	};
#define NUM_EVNT ((sizeof(evnt)/sizeof(struct evnt)))


/* build audit record body: service, result, validation mechanism,	*/
/* service event, remote and local system and user, plus free string	*/


/* put service into audit body	*/
strcpy(body, "\n\tNetworking service = ");
if (!(NA_MASK_SERV & ignore_mask))
{
	strncat(body, service, SERV_SZ); /* in case string is too long */
	strcat(body, "\n");
}
else
	strcat(body, unspecified);


/* put audit result (success or failure) into audit body	*/
strcpy(line, "\tRequest outcome    = ");
if (!(NA_MASK_RSLT & ignore_mask))
	if ((result != NA_RSLT_SUCCESS) && (result != NA_RSLT_FAILURE))
	{
		/* put the actual result code into the audit file */
		strcat(line, "failure (");
		strcat(line, ultoa(result));
		strcat(line, ")\n");
	}
	else
	{
		strcat(line, rslt[result].name);
		strcat(line, "\n");
	}
else
	strcat(line, unspecified);
strcat (body, line);


/* put network audit validation mechanism into audit record	*/
strcpy(line, "\tValidation tool    = ");
if ((NA_VALI_OTHER <= valid_mech) && (NA_VALI_LAST >= valid_mech)
	&& (!(NA_MASK_VALI & ignore_mask)))
{
	strcat(line, vali[valid_mech].name);
	strcat(line, "\n");
}
else
	strcat(line, unspecified);
strcat (body, line);


/* put network audit service evnt into audit record	*/
strcpy(line, "\tService event      = ");
if ((NA_EVNT_OTHER <= event) && (NA_EVNT_LAST >= event)
	&& !(NA_MASK_EVNT & ignore_mask))
{
	strcat(line, evnt[event].name);
	strcat(line, "\n");
}
else
	strcat(line, unspecified);
strcat (body, line);


/* put remote system into audit record	*/
strcpy(line, "\tRemote system      = ");
if (!(NA_MASK_RSYS & ignore_mask))
{
	strcat(line, inet_ntoa(remote_system));
	strcat(line, "\n");
}
else
	strcat(line, unspecified);
strcat (body, line);


/* put remote user   into audit record	*/
strcpy(line, "\tRemote user        = ");
if (!(NA_MASK_RUSR & ignore_mask))
{
	strcat(line, ultoa(remote_user));
	strcat(line, "\n");
}
else
	strcat(line, unspecified);
strcat (body, line);


/* put local  system into audit record	*/
strcpy(line, "\tLocal  system      = ");
if (!(NA_MASK_LSYS & ignore_mask))
{
	strcat(line, inet_ntoa(local_system));
	strcat(line, "\n");
}
else
	strcat(line, unspecified);
strcat (body, line);


/* put local  user   into audit record	*/
strcpy(line, "\tLocal  user        = ");
if (!(NA_MASK_LUSR & ignore_mask))
{
	strcat(line, ultoa(local_user));
	strcat(line, "\n");
}
else
	strcat(line, unspecified);
strcat (body, line);


/* put free string into audit body	*/
if (!(NA_MASK_FREE & ignore_mask) && (free_field != (char *)0))
	strcat(body, free_field);

/* build audit record header    */
audrec.aud_head.ah_error = result;

if (NA_EVNT_STOP  == event)
        audrec.aud_head.ah_event = EN_IPCCLOSE;
else if (NA_EVNT_PASSWD == event)
        audrec.aud_head.ah_event = EN_PASSWD;
else /* log all other event types under IPCOPEN */
        audrec.aud_head.ah_event = EN_IPCOPEN;

audrec.aud_head.ah_len   = strlen (body);


/* deliver record to audit subsystem    */
audwrite (&audrec);
}

