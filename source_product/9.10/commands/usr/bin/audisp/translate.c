#include <string.h>
#include <sys/types.h>
#include <sys/audit.h>
#include <pwd.h>
#include <grp.h>

/*******************************************************************************
 *           T R A N S L A T E . C
 *
 * Name translation code for Audisp(1M)
 *   event number -> event name
 *   audit id     -> user name
 *   real gid     -> real group name
 *   eff. gid     -> effective group name
 ******************************************************************************/

/* Routines in translate.c */

char *geten();
char *getun();
char *getgn();
char *eton();
char *uton();
char *gton();
char *strsave();

/*
 * event to name mapping
 */
struct ev_map {
   int ev_num;
   char *ev_name;
};

/*
 * syscall to name mapping
 */
struct sc_map {
   int sc_num;
   int ev_class;
   char *sc_name;
};

extern int scmapsize;
extern int evmapsize;
extern struct sc_map sc_map[];
extern struct ev_map ev_map[];

/* Global data structures */

struct evmapent {
	int ev;
	char *en;
	struct evmapent *next;
};

struct usermapent {
	aid_t aid;
	char *un;
	struct usermapent *next;
};

struct grpmapent {
	gid_t gid;
	char *gn;
	struct grpmapent *next;
};

struct evmapent *evmap;
struct evmapent *evmaptail;

struct usermapent *usermap;
struct usermapent *usermaptail;

struct grpmapent *grpmap;
struct grpmapent *grpmaptail;

/*******************************************************************************
 * G E T E N
 * maintains known event-name translation on a linked list; calls
 * eton() to get unknown ones.
 ******************************************************************************/
char *geten (ev_num)
int ev_num;
{
	struct evmapent *ep;

	ep = evmap;
	while ( ep != (struct evmapent *) NULL ) {
		if ( ep->ev == ev_num )
			break;
		ep = ep->next;
	}
	if ( ep == (struct evmapent *) NULL ) {
		/* ev_num not found in linked list */
		ep = (struct evmapent *)
		     malloc ( sizeof ( struct evmapent ) );
		ep->ev = ev_num;
		ep->en = strsave ( eton ( ev_num ) );
		ep->next = ( struct evmapent *) NULL;
		if ( evmaptail != (struct evmapent *) NULL )
			/* not the first one in linked list */
			evmaptail->next = ep;
		evmaptail = ep;
		if ( evmap == (struct evmapent *) NULL )
			/* first one in linked list */
			evmap = ep;
	}
	return ( ep->en );
}

/*******************************************************************************
 * G E T U N
 * maintains known user-name translation on a linked list; calls
 * uton() to get unknown ones.
 ******************************************************************************/
char *getun (aid)
aid_t aid;
{
	struct usermapent *up;

	up = usermap;
	while ( up != (struct usermapent *) NULL ) {
		if ( up->aid == aid )
			break;
		up = up->next;
	}
	if ( up == (struct usermapent *) NULL ) {
		/* aid not found in linked list */
		up = (struct usermapent *)
		     malloc ( sizeof ( struct usermapent ) );
		up->aid = aid;
		up->un = strsave ( uton ( aid ) );
		up->next = ( struct usermapent *) NULL;
		if ( usermaptail != (struct usermapent *) NULL )
			/* not the first one in linked list */
			usermaptail->next = up;
		usermaptail = up;
		if ( usermap == (struct usermapent *) NULL )
			/* first one in linked list */
			usermap = up;
	}
	return ( up->un );
}

/*******************************************************************************
 * G E T G N
 * maintains known group-name translation on a linked list; calls
 * gton() to get unknown ones.
 ******************************************************************************/
char *getgn (gid)
gid_t gid;
{
	struct grpmapent *gp;

	gp = grpmap;
	while ( gp != (struct grpmapent *) NULL ) {
		if ( gp->gid == gid )
			break;
		gp = gp->next;
	}
	if ( gp == (struct grpmapent *) NULL ) {
		/* gid not found in linked list */
		gp = (struct grpmapent *)
		     malloc ( sizeof ( struct grpmapent ) );
		gp->gid = gid;
		gp->gn = strsave ( gton ( gid ) );
		gp->next = ( struct grpmapent *) NULL;
		if ( grpmaptail != (struct grpmapent *) NULL )
			/* not the first one in linked list */
			grpmaptail->next = gp;
		grpmaptail = gp;
		if ( grpmap == (struct grpmapent *) NULL )
			/* first one in linked list */
			grpmap = gp;
	}
	return ( gp->gn );
}

/*******************************************************************************
 * E T O N
 * given an event number, look up the name in ev_map or sc_map (eventmap.h)
 ******************************************************************************/
char *
eton ( ev_num )
int ev_num;
{
	int i;
	int ec;
	int foundsc, foundev;

	foundsc = foundev = 0;

	if ( ( ec = ( ev_num & ~01777 ) ) != 0 ) {
		for ( i = 0; i < evmapsize; i++ )
			if ( ec == ev_map[i].ev_num ) {
				foundev++;
				break;
			}
	} else {
		for ( i = 0; i < scmapsize; i++ )
			if ( ev_num == sc_map[i].sc_num ) {
				foundsc++;
				break;
			}
	}
	if ( foundsc ) {
		return ( sc_map[i].sc_name );
	} else if ( foundev ) {
		return ( ev_map[i].ev_name );
	} else {
		return ("????????");	
	}
}

/*******************************************************************************
 * U T O N
 * given an audit id, look up the user's name.
 ******************************************************************************/
char *
uton ( aid )
aid_t aid;
{
	struct s_passwd *pwd, *getspwent();

	setspwent();
	while ( ( pwd = getspwent() ) !=
		( struct s_passwd * ) NULL )
		if ( pwd->pw_audid == aid )
			break;
	if ( pwd == ( struct s_passwd * ) NULL )
		/* name not found */
		return ("????????");	
	return ( pwd->pw_name );
}

/*******************************************************************************
 * G T O N
 * given a group id, look up the group name.
 ******************************************************************************/
char *
gton ( gid )
gid_t gid;
{
	struct group *grp, *getgrent();

	setgrent();
	while ( ( grp = getgrent() ) !=
		( struct group * ) NULL )
		if ( grp->gr_gid == gid )
			break;
	if ( grp == ( struct group * ) NULL )
		/* group name not found */
		return ("????????");	
	return ( grp->gr_name );
}

/*******************************************************************************
 * S T R   S A V E
 *
 * Given a string pointer, copy the old string into malloc'd space and
 * return a pointer to the new copy.  Error out if malloc() fails.
 ******************************************************************************/
char *
strsave (string)
char *string;
{
	return (strcpy (malloc (strlen (string) + 1), string));
} /* strsave */
