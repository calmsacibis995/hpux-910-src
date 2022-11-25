/* $Revision: 64.1 $ */
/*
 * Get the command of alteration from remote host.
 */

#include "lp.h"

extern char work[BUFSIZ];
extern char *printer;

char dst[DESTMAX+1];
int seqno;
char host[SP_MAXHOSTNAMELEN];

char title[TITLEMAX+1];
char opts[OPTMAX];
int copies;
short priority;
short mail;
short wrt;

char	user_name[LOGMAX+1];

/* alter -- get alter command from remote and set the grobal variables */

alter()
{
	int	size;
	char	*cp, *head;
	char	line[BUFSIZ];

	/* ack of the request */

	write(1, "", 1);

	for(;;){
		/* Read a command to tell us what to do */

		cp = line;
		do{
			if( (size = read(0, cp, 1)) != 1){
				if(size < 0)
				    fatal("Lost connection");
				return(0);
			}
		}while(*cp++ != '\n');
		*--cp = '\0';
		cp = line;
		fprintf(stderr, "%s\n", cp);

		switch(*cp++){

		    case '\1' :
			write(1, "", 1);
			set_user_id(1);	/* set euid to LP admin */
			strcpy(dst, printer);
			altother(FALSE);
			set_user_id(0);	/* set euid to root */
			return(0);

		    case '\2' :
			/* get the request id */
			head = cp;
			while(*cp && *cp != ' ')
			    cp++;
			if(!*cp)
			    fatal("Connection is down", 1);
			*cp++ = '\0';
			strcpy(user_name, head);

			head = cp;
			while(*cp && *cp != ' ')
			    cp++;
			if(!*cp)
			    fatal("Connection is down", 1);
			*cp++ = '\0';
			seqno = atoi(head);

			strcpy(host, cp);			
			break;

		    case R_COPIES:
			copies = atoi(cp);
			break;
		    case R_HEADERTITLE:
			strcpy(title, cp);
			break;
		    case R_OPTIONS:
			strcpy(opts, cp);
			break;
		    case R_MAIL:
			mail = TRUE;
			break;
		    case R_WRITE:
			wrt = TRUE;
			break;
		    default:
			break;
		}
	}
}
