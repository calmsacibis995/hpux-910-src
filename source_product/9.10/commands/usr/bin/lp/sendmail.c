/* $Revision: 66.1 $ */
/* sendmail(user, msg) -- send msg to user's mailbox */

#include	"lp.h"

#ifdef REMOTE
sendmail(user, remote_host, msg)
#else
sendmail(user, msg)
#endif REMOTE
char	*user;
#ifdef REMOTE
char	*remote_host;
#endif REMOTE
char	*msg;
{
	FILE *pfile, *popen();
#ifdef REMOTE
	char mailcmd[LOGMAX + SP_MAXHOSTNAMELEN + 12];
	char	local_host[SP_MAXHOSTNAMELEN];

	gethostname(local_host, sizeof (local_host));
#else
	char mailcmd[LOGMAX + 11];
#endif REMOTE

	if(isnumber(user))
		return;

	if(GET_ACCESS("/bin/mail", ACC_X) != 0)
	        return;

#ifdef REMOTE
	if (strcmp(local_host, remote_host))
		sprintf(mailcmd, "/bin/mail %s@%s", user, remote_host);
	else
#endif REMOTE
		sprintf(mailcmd, "/bin/mail %s", user);
	if((pfile = popen(mailcmd, "w")) != NULL) {
		fprintf(pfile, "%s\n", msg);
		pclose(pfile);
	}
}

isnumber(s)
char *s;
{
	int	c;

	while((c = *(s++)) != '\0')
		if(c < '0' || c > '9')
			return(FALSE);
	return(TRUE);
}
