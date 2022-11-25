/*
 * Local Stuff
 *
 * $Header: local.c,v 4.5 86/07/10 11:00:35 boothby Exp $
 * Copyright Hewlett Packard Co. 1986
 */

#include <stdio.h>
#include <pwd.h>
#include <string.h>

extern struct passwd *getpwnam();
extern struct passwd *getpwuid();
extern unsigned short getuid();

/*
 * Return the pointer to the last non-whitespace character in string s.
 */

char *
lastnws(s)
	register char *s;		/* string to search */
{
        register char *r;		/* ptr to last non-whitespace char */

        r = s;
        do	{
                if (*s != ' ' && *s != '\t' && *s != '\n' && *s != '\0')
                        r = s;
        	} while (*s++);
        return(r);
}

/*
 * return fullname of user (found in gecos field)
 * example:
 *	fullname("kg") returns "Ken Greer"
 */
char *
fullname (login)
char *login;
{
    register char *ip, *op, *lp;
    static char name[50];
    struct passwd *pw = getpwnam (login);

    if (pw == NULL)
	return (NULL);
    for (op = name, ip = pw -> pw_gecos; *ip && *ip != ','; ip++) {
	if (*ip == '&') {
	    lp = login;
	    if (('a' <= *lp) && (*lp <= 'z'))
		*op++ = *lp++ - 040;
	    else
		*op++ = *lp++;
	    while (*lp && *lp != ' ')
		*op++ = *lp++;
	} else
	    *op++ = *ip;
    }
    *op = '\0';
    op = lastnws(name);
    *++op = '\0';
    return (name);
}

/*
 * Return login name of current "real" user
 */
char *
getuser ()
{
    struct passwd *eff_user;
    static char user_name[L_cuserid];

    if ((eff_user = getpwuid(getuid())) == NULL)
	faterror("cannot get real user name!");
    strcpy(user_name, eff_user->pw_name);
    return (user_name);
}
