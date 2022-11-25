/* @(#)$Header: getservent.c,v 70.1.1.1 94/10/04 11:06:44 hmgr Exp $ */

/* 
 * unlike gethost, getpw, etc, this doesn't implement getservbyxxx
 * directly
 */

#ifdef _NAMESPACE_CLEAN
#define fclose _fclose
#define fopen  _fopen
#define fgets  _fgets
#define fprintf _fprintf
#define sprintf _sprintf
#define yp_first _yp_first
#define yp_match _yp_match
#define yp_next _yp_next
#define getdomainname _getdomainname
#define usingyellow _usingyellow
#define yellowup _yellowup
#define ultoa _ultoa
#define strlen _strlen
#define strcat _strcat
#define strcmp _strcmp
#define strcpy _strcpy
#define strncpy _strncpy
#define atoi _atoi
#define	rewind _rewind
#ifdef	_ANSIC_CLEAN
#define	malloc _malloc
#define	free _free
#endif
/* local */
#define getservbyport _getservbyport
#define getservbyname _getservbyname
#define setservent _setservent
#define endservent _endservent
#define getservent _getservent
#define yp_ismapthere _yp_ismapthere
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <ctype.h>
#ifdef HP_NFS
#include <rpcsvc/ypclnt.h>
#endif HP_NFS

/*
 * Internet version.
 */
#define	MAXALIASES	35
#define	MAXADDRSIZE	14
#define LINEBUFSIZ	1024

static char SERVDB[] = "/etc/services";
static FILE *servf = NULL;
static int stayopen = 0;
static struct servent *interpret();
struct servent *getservent();
static char *any();
#ifdef HP_NFS
static char domain[256];
static char *current = NULL;	/* current entry, analogous to servf */
static int currentlen;
extern int usingyellow;		/* are Yellow Pages up? */
extern int yellowup();		/* are Yellow Pages up? */
#endif HP_NFS


/*
* Handy utility function that other libc routines can call to
* see that are getting data from a NIS map.
* e.g. if a yp_match fails and the stat is checked by this
* routine FALSE is a fatal error and you should stop using NIS
* TRUE means that the map exists and you should return
* the error.
*/

#ifdef _NAMESPACE_CLEAN
#undef yp_ismapthere
#pragma _HP_SECONDARY_DEF _yp_ismapthere yp_ismapthere
#define yp_ismapthere _yp_ismapthere
#endif

#ifndef TRUE
#	define	TRUE (1)
#endif
#ifndef FALSE
#	define	FALSE (0)
#endif

int
yp_ismapthere(stat)
       int stat;
{

      switch (stat) {

      case 0:  /* it actually succeeded! */
      case YPERR_KEY:  /* no such key in map */
      case YPERR_NOMORE:
             return (TRUE);
      }
      return (FALSE);
}

#ifdef _NAMESPACE_CLEAN
#undef getservbyport
#pragma _HP_SECONDARY_DEF _getservbyport getservbyport
#define getservbyport _getservbyport
#endif

struct servent *
getservbyport(port, proto)
	int port;
	char *proto;
{
	register struct servent *p;
#ifdef HP_NFS
	int reason;
	char key[255];
	char *val;
	int vallen;
#endif HP_NFS
	extern char *ultoa();


	setservent(0);
#ifdef HP_NFS
	/*
	**      if using YP, first try to yp_match the port/proto key in
	**	the services.byname map, since the de facto standard is
	**      actually to hash services.byname by port/proto.  If that
	**	fails, sequentially search the services.byname map.
	*/

	if (usingyellow) {
		sprintf(key, "%d/%s", port, proto);
		reason =  yp_match(domain, "services.byname",
		    key, strlen(key), &val, &vallen);
#ifdef DEBUG
		fprintf(stderr, "reason yp_match(port/proto,services.byname) failed is %d\n",
			reason);
#endif DEBUG
		switch (reason) {
			case 0:
				p = interpret(val, vallen);
				free (val);
				return (p);

			case YPERR_KEY:
				break;

			case YPERR_MAP:
			     usingyellow = 0;
			     break;

			default:
				return(NULL);
		}
	}
#endif HP_NFS
	while (p = getservent()) {
		if (p->s_port != port)
			continue;
		if (proto == 0 || strcmp(p->s_proto, proto) == 0)
			break;
	}
	endservent();
	return (p);
}



#ifdef _NAMESPACE_CLEAN
#undef getservbyname
#pragma _HP_SECONDARY_DEF _getservbyname getservbyname
#define getservbyname _getservbyname
#endif

struct servent *
getservbyname(name, proto)
	register char *name, *proto;
{
	register struct servent *p;
	register char **cp;
#ifdef HP_NFS
	int reason;
	char key[255];
	char *val;
	int vallen;
#endif HP_NFS

	setservent(0);

#ifdef HP_NFS
	/*
	**      if using YP, first try to yp_match the name/proto key in
	**	the services.bynp map. This is a new HP map that uses 
	**	service_name/proto as the key.
	*/

	if (usingyellow) {
		if (proto != NULL)  {
			sprintf(key, "%s/%s", name, proto);
			reason =  yp_match(domain, "servi.bynp", key, 
			                   strlen(key), &val, &vallen);

		} else { /* if proto = NULL, look up tcp first */
			sprintf(key, "%s/tcp", name); /* try tcp first */
			reason =  yp_match(domain, "servi.bynp", key, 
			                   strlen(key), &val, &vallen);
			if (reason == YPERR_KEY) { /* lookup udp */
				sprintf(key, "%s/udp", name);
				reason =  yp_match(domain, "servi.bynp", key, 
				                   strlen(key), &val, &vallen);
			}
		}

		if (reason == 0)
		{
			p = interpret(val, vallen);
			free(val);
			return (p);
		}
	}
#endif HP_NFS

       	while (p = getservent()) {
               	if (strcmp(name, p->s_name) == 0)
                       	goto gotname;
               	for (cp = p->s_aliases; *cp; cp++)
                       	if (strcmp(name, *cp) == 0)
                               	goto gotname;
               	continue;
gotname:
        	if (proto == 0 || strcmp(p->s_proto, proto) == 0)
                	break;
       	}

        endservent();
        return (p);
}

#ifdef _NAMESPACE_CLEAN
#undef setservent
#pragma _HP_SECONDARY_DEF _setservent setservent
#define setservent _setservent
#endif

setservent(f)
	int f;
{
#ifdef HP_NFS
	if (getdomainname(domain, sizeof(domain)) >= 0) {
		yellowup(1, domain);	/* recompute whether Yellow Pages are up */
	}
	if (!usingyellow) {
#endif HP_NFS
	if (servf == NULL)
		servf = fopen(SERVDB, "r");
	else
		rewind(servf);
#ifdef HP_NFS
	}
	if (current)
		free(current);
	current = NULL;
#endif HP_NFS
	stayopen |= f;
}



#ifdef _NAMESPACE_CLEAN
#undef endservent
#pragma _HP_SECONDARY_DEF _endservent endservent
#define endservent _endservent
#endif

endservent()
{
#ifdef HP_NFS
	if (current && !stayopen) {
		free(current);
		current = NULL;
	}
#endif HP_NFS
	if (servf && !stayopen) {
		fclose(servf);
		servf = NULL;
	}
}



#ifdef _NAMESPACE_CLEAN
#undef getservent
#pragma _HP_SECONDARY_DEF _getservent getservent
#define getservent _getservent
#endif

struct servent *
getservent()
{
	static char line1[LINEBUFSIZ+1];
	struct servent *sp;
#ifdef HP_NFS
	int reason;
	char *key, *val;
	int keylen, vallen;

	yellowup(0, domain);
	if (!usingyellow) {
#endif HP_NFS
		if (servf == NULL && (servf = fopen(SERVDB, "r")) == NULL)
			return (NULL);
	        if (fgets(line1, LINEBUFSIZ, servf) == NULL)
			return (NULL);
		return interpret(line1, strlen(line1));
#ifdef HP_NFS
	}
	if (current == NULL) {
		if (reason =  yp_first(domain, "services.byname",
		    &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_first failed is %d\n",
				reason);
#endif DEBUG
			if(yp_ismapthere(reason) == 0)  {
			   usingyellow = 0;
			   return(getservent());
			   }
			return NULL;
		    }
	}
	else {
		if (reason = yp_next(domain, "services.byname",
		    current, currentlen, &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_next failed is %d\n",
				 reason);
#endif DEBUG
                        if(yp_ismapthere(reason) == 0)  {
			   usingyellow = 0;
 		           return(getservent());
			   }
			return NULL;
		}
	}
	if (current)
		free(current);
	current = key;
	currentlen = keylen;
	sp = interpret(val, vallen);
	free(val);
	return (sp);
#endif HP_NFS
}



static struct servent *
interpret(val, len)
{
	static char *serv_aliases[MAXALIASES];
	static struct servent serv;
	static char line[LINEBUFSIZ+1];
	char *p;
	register char *cp, **q;

	strncpy(line, val, len);
	p = line;
	line[len] = '\n';
	if (*p == '#')
		return (getservent());
	cp = any(p, "#\n");
	if (cp == NULL)
		return (getservent());
	*cp = '\0';
	serv.s_name = p;
	p = any(p, " \t");
	if (p == NULL)
		return (getservent());
	*p++ = '\0';
	while (*p == ' ' || *p == '\t')
		p++;
	cp = any(p, ",/");
	if (cp == NULL)
		return (getservent());
	*cp++ = '\0';
	serv.s_port = htons((u_short)atoi(p));
	serv.s_proto = cp;
	q = serv.s_aliases = serv_aliases;
	cp = any(cp, " \t");
	if (cp != NULL)
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &serv_aliases[MAXALIASES - 1])
			*q++ = cp;
		cp = any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&serv);
}



static char *
any(cp, match)
	register char *cp;
	char *match;
{
	register char *mp, c;

	while (c = *cp) {
		for (mp = match; *mp; mp++)
			if (*mp == c)
				return (cp);
		cp++;
	}
	return ((char *)0);
}

