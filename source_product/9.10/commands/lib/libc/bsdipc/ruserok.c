
/* @(#)$Header: ruserok.c,v 66.4.1.1 94/10/11 14:24:09 hmgr Exp $ */

/*
**	hosts.equiv and .rhosts authentication
*/

#ifdef _NAMESPACE_CLEAN
#define close _close
#define fileno _fileno
#define fclose _fclose
#define fgets  _fgets
#define fopen  _fopen
#define fstat _fstat
#define geteuid _geteuid
#define lstat _lstat
#define setresuid _setresuid
#define stat _stat
#define strcat _strcat
#define strcpy _strcpy
#define strlen _strlen
#define strcmp _strcmp
#define strtok _strtok
#define strchr _strchr
#define _exit ___exit
#define getdomainname _getdomainname
#define getpwnam _getpwnam
#define innetgr _innetgr
#define perror _perror
#define printf _printf
#define res_init _res_init
#define strcasecmp _strcasecmp
#define strncasecmp _strncasecmp
#define cnodes _cnodes
#define getcccid _getcccid
/* local */
#define ruserok _ruserok
/* NLS */
#ifdef NLS
#define catgets _catgets
#define catopen _catopen
#endif
#endif _NAMESPACE_CLEAN



#ifndef NLS
#define catgets(i,sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS


#include <memory.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>

#include <netinet/in.h>

#include <netdb.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <errno.h>
#include <string.h>

#include	<sys/stat.h>
#include	<pwd.h>
/*
**	check to see if the remote user is OK -- validates user using the
**	/etc/hosts.equiv file (unless root), and $HOME/.rhosts 
*/
#ifdef _NAMESPACE_CLEAN 
#undef ruserok
#pragma _HP_SECONDARY_DEF _ruserok ruserok
#define ruserok _ruserok
#endif _NAMESPACE_CLEAN

extern int _check_rhosts_file;

ruserok(rhost, superuser, ruser, luser)
	char *rhost;
	int superuser;
	char *ruser, *luser;
{
	FILE *hostf;
	char line[BUFSIZ];
	char *ahost, *user, *cp;
	int first = 1;
	int hostmatch, usermatch;
	int baselen = -1;
	char *p;
#ifdef NLS
	nl_catd nlmsg_fd;
#endif /* NLS */
#ifdef HP_NFS
	char domain[256];
#endif HP_NFS

#ifdef NLS
	nlmsg_fd = catopen("rcmd",0);
#endif NLS

	if ((p = strchr(rhost, '.')) != NULL)
	{
		baselen = p - rhost;
	}
#ifdef HP_NFS
	if (getdomainname(domain, sizeof(domain)) < 0) {
		perror(catgets(nlmsg_fd,NL_SETN,16, "ruserok: getdomainname"));
		_exit(1);
	}
#endif HP_NFS

	hostf = superuser ? (FILE *)0 : fopen("/etc/hosts.equiv", "r");
again:
	if (hostf) {
		while (cp = fgets(line, sizeof (line), hostf)) 
		{
			hostmatch = usermatch = 0;
			if (*cp == '#')
				continue;	/* skip comments */
			ahost = strtok(cp," \t\n");		
			if (ahost == NULL)
				continue;	/* skip blank lines */
			user = strtok(NULL," \t\n");
			if (ahost[0] == '%' && ahost[1] == 0)
				hostmatch = _incluster(rhost);
#ifdef HP_NFS
			else if (ahost[0] == '+' && ahost[1] == 0)
				hostmatch = 1;
			else if (ahost[0] == '+' && ahost[1] == '@')
				hostmatch = innetgr(ahost + 2, rhost,
				    NULL, domain);
			else if (ahost[0] == '-' && ahost[1] == '@') {
				if (innetgr(ahost + 2, rhost, NULL, domain))
					break;
			}
			else if (ahost[0] == '-') {
				if (_checkhost(rhost, ahost+1, baselen))
					break;
			}
#endif HP_NFS
			else
				hostmatch = _checkhost(rhost, ahost, baselen);
			if (hostmatch && user) {
#ifdef HP_NFS
				if (user[0] == '+' && user[1] == 0)
					usermatch = 1;
				else if (user[0] == '+' && user[1] == '@')
					usermatch = innetgr(user+2, NULL,
					    ruser, domain);
				else if (user[0] == '-' && user[1] == '@') {
					if (innetgr(user+2, NULL,
					    ruser, domain))
						break;
				}
				else if (user[0] == '-') {
					if (!strcmp(user+1, ruser))
						break;
				}
				else
#endif HP_NFS
					usermatch = !strcmp(ruser, user);
			}
			else
				usermatch = !strcmp(ruser, luser);
			if (hostmatch && usermatch) {
				(void) fclose(hostf);
				return (0);
			}
		}
		(void) fclose(hostf);
	}
	if (first == 1 && (_check_rhosts_file || superuser)) {
		char	*rhosts=line;
		struct	stat sbuf;
		uid_t   euid;
		struct	passwd	*pw, *getpwnam();

		first = 0;
		/*
		**	determine the local user's home directory and uid
		*/
		if ((pw=getpwnam(luser)) == (struct passwd *)NULL)
			goto bad;
		strcpy(line, pw->pw_dir);
		strcat(line, "/.rhosts");
#ifndef SYMLINKS
		if (stat(rhosts, &sbuf) < 0)
			goto bad;
#else SYMLINKS
		if (lstat(rhosts, &sbuf) < 0)
			goto bad;
		if ((sbuf.st_mode & S_IFMT) == S_IFLNK) {
			printf((catgets(nlmsg_fd,NL_SETN,17, "Warning: .rhosts is a soft link.\r\n")));
			goto bad;
		}
#endif SYMLINKS
                /*
                   Open .rhosts with the real user as effective user.
                   (on NFS mounted file systems "root" may not be
                   allowed to read files with read/write permissions
                   for the owner only).
		   Change by Valentin Barazan.  Ported to s300 by Craig Bryant.
                */

                euid = geteuid();
                (void) setresuid(-1, pw->pw_uid, -1);
                hostf = fopen(rhosts, "r");
                (void) setresuid(-1, euid, -1);
		if (hostf == NULL)      /* shouldn't happen */
			goto bad;

		fstat(fileno(hostf), &sbuf);
		if (sbuf.st_uid && (sbuf.st_uid != pw->pw_uid)) {
			printf((catgets(nlmsg_fd,NL_SETN,18, "Warning: Bad .rhosts ownership.\r\n")));
			fclose(hostf);
			goto bad;
		}
		goto again;
	}
bad:
	return (-1);
}



static int
_checkhost(rhost, lhost, len)
	char *rhost, *lhost;
	int len;
{
	static char *domainp = NULL;
	static int nodomain = 0;
	extern struct state _res;

	if (len == -1)
		/* no domain on rhost, must match lhost exactly */
		return(!strcasecmp(rhost, lhost));
	if (strncasecmp(rhost, lhost, len))
		/* host name local parts don't match, why go on? */
		return(0);
	if (!strcasecmp(rhost, lhost))
		/* whole domain-extended names match, succeed */
		return(1);
	if (*(lhost + len) != '\0')
		/* lhost has a domain but it didn't match, fail */
		return(0);
	if (nodomain)
		/* already determined there is no local domain, fail */
		return(0);
	if (!domainp) {
		/*
		**  local domain not returned in hostname(),
		**  get it from _res.defdname
		*/
		if (!(_res.options & RES_INIT))
			if (res_init() == -1) {
				/* there is no local domain */
				nodomain = 1;
				return(0);
			}
		domainp = _res.defdname;
	}
	/* rhost's domain must match local domain */
	return(!strcasecmp(domainp, rhost + len + 1));
}


#include <cluster.h>

static int
_incluster(rhost)
	char *rhost;
{
	cnode_t buf[MAX_CNODE]; 
        int n, i;
	struct cct_entry *getcccid(), *cct;

	if ((n = cnodes(buf)) <= 0)
	{
		/*
		**  < 0 means error;
		**  = 0 means standalone system;
		**  in either case, fail
		*/
		return(0);
	}
	else
	{
		for (i = 0; i < n; i++)
		{
			/* assume all cnodes in cluster in local domain */
			if ((cct = getcccid(buf[i])) &&
			    _checkhost(rhost, cct->cnode_name,
			    strlen(cct->cnode_name)))
			{
				return(1);
			}
		}
		return(0);
	}
}
