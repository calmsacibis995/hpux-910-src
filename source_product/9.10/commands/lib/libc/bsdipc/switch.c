/*
 *	Copyright (c) 1993  Hewlett-Packard Company
 *                  ALL RIGHTS RESERVED
 *
 *  @(#) switch.c $Header: switch.c,v 70.5 94/02/24 14:08:40 ssa Exp $
 *
 *  This file contains the routines for handling the parsing of the nameservice
 *  switch file (nsswitch.conf) and the creation of a policy from any of its
 *  valid configuration lines. The switch is used by routines such as
 *  gethostbyname() to find what service is the primary name service and
 *  what other services are available for use when the primary name service 
 *  is unavailable for a variety of reasons.
 *
 *  THREAD SAFING NOTE:  This module in itself is not thread safe. This
 *  routine will probably be called only once for each database type. The
 *  data returned will probably be valid for all threads, thus mutex's are
 *  used. A mutex in libc is available called _switch_rmutex. 
 *
 *  The usage within a thread can look like this:
 *
 *	_rec_mutex_lock(&_switch_rmutex);
 *	if (policy == NULL )  {
 *	    if ( (policy = __nsw_getconfig("XXX", &status)) == NULL)
 *		policy = __nsw_getdefault("XXX");
 *      _rec_mutex_unlock(&_switch_rmutex);
 *
 *      policy can be a shared/global variable visible to all threads.
 */

#ifdef _NAMESPACE_CLEAN
#define fprintf _fprintf
#define printf _printf
#define strcpy _strcpy
#define strlen _strlen
#define strcmp _strcmp
#define strchr _strchr
#define fopen _fopen
#define fclose _fclose
#define fgets _fgets
#define isalpha _isalpha
#define tolower __tolower
#define isupper _isupper
#ifdef    _ANSIC_CLEAN
#define    malloc _malloc
#define    free _free
#endif    /* _ANSIC_CLEAN */
#endif /* _NAMESPACE_CLEAN */


#include  <stdio.h>
#include  <string.h>
typedef enum  boolean {
	false=0, true=1
} 
BOOL;
#include  "switch.h"

#define  LINSIZ		256		/* Max # of chars in a line */
#define  __NSW_TRASH	-1		/* To track a trash status-id */

/*
 * exported symbols
 */
int		__nsw_debug = 0;	/* debugging level: 0=disable,  1=enable
					 * currently used by nslookup  */
/*
 * local symbols
 */
static  char	*module = "__nsw";	/* modulename */

static  char	mytext[LINSIZ];		/* scanned text symbol */
static  int	mylineno = 0;		/* current line no */
static  char	iline[LINSIZ];		/* current input line */
static  char	*rlinep;		/* pointer to unscanned portion of the line */
static  FILE	*myin;			/* input file pointer */

static  int	Garbage	= false;	/* to handle [X=garbage] criterion */
static  struct  __nsw_lookup	*st_lkup = NULL;	/* start of lookups */
static  struct  __nsw_lookup	*res_lookup = NULL;	/* result of lookup policy parse */
/*
 *	the following arrays maintain links to allocated structures
 *	for cleanup during error recovery.
 */
#define  MXLKUPS	10		/* current max # of sources per info-class/database */
static  int		num_id = 0;	/* count of memalloced ids */
static  int		num_lookup = 0;	/* count of memalloced lookups */
static  char		*ar_id[MXLKUPS];
static  T_NSW_LOOKUP	*ar_lookup[MXLKUPS];


/*----------------------------------------------------------------------
 * Submodule: action utilities
 *	reset_vars()
 *	alc_id()
 *	alc_lookup()
 *	free_lookup()
 *	FixAActions()
 *	FFixLookUp()
 *	error_recovery()
 ---------------------------------------------------------------------*/

/*
 * reset_vars:  to ensure reentrancy reset switch's variables
 */
static  int
reset_vars ()
{
	int	ii;

	res_lookup = st_lkup = NULL;
	num_lookup = 0;
	num_id = 0;
	for (ii = 0; ii < MXLKUPS; ii++) {
		ar_id[ii] = NULL;
		ar_lookup[ii] = NULL;
	}
}

/*
 * alc_id:	allocates a string of required size and maintains an
 *	array of allocated strings for cleanup.
 */
static  char  *
alc_id (Siz)
int  Siz;
{
	if (num_id >= MXLKUPS) {
		if (__nsw_debug)
			printf("%s.alc_id: ERR- Number of Sources > %d\n", module, MXLKUPS);
		return ((char  *)NULL);
	}
	return(ar_id[num_id++] = (char *)malloc(Siz));
}

/*
 * alc_lookup:	allocates and initialises a lookup structure and
 *	maintains an array of allocated structures for cleanup.
 */
static  struct	__nsw_lookup  *
alc_lookup ()
{
	T_NSW_LOOKUP *	temp_lookup;

	if (num_lookup >= MXLKUPS) {
		if (__nsw_debug)
			printf("%s.alc_lookup: ERR- Number of Sources > %d\n", module, MXLKUPS);
		return ((struct  __nsw_lookup  *)NULL);
	}
	if ( temp_lookup=(T_NSW_LOOKUP *)malloc(sizeof(T_NSW_LOOKUP)) ) {
		temp_lookup->service_name = NULL;
		/*
		 * init default actions
		 */
		temp_lookup->action[__NSW_SUCCESS] = __NSW_RETURN;
		temp_lookup->action[__NSW_NOTFOUND] = __NSW_RETURN;
		temp_lookup->action[__NSW_UNAVAIL] = __NSW_CONTINUE;
		temp_lookup->action[__NSW_TRYAGAIN] = __NSW_RETURN;
		temp_lookup->long_errs = NULL;
		temp_lookup->next = NULL;
		return(ar_lookup[num_lookup++] = temp_lookup);
	}
	return((T_NSW_LOOKUP *)NULL);
}

/*
 * free_lookup:	self_explanatory (need we say more ?).
 */
static  int
free_lookup (p_lookup)
T_NSW_LOOKUP	*p_lookup;
{

	if (p_lookup->service_name != NULL)
		free(p_lookup->service_name);
	free(p_lookup);
	return(1);
}

/*
 * FixAActions:	 Handy routine for assigning same Action to all
 *	statuses.
 */
static  int
FixAActions (Llkup, Action)
T_NSW_LOOKUP	*Llkup;
action_t		Action;
{
	Llkup->action[__NSW_SUCCESS] = Action;	/* override */
	Llkup->action[__NSW_NOTFOUND] = Action;	/* override */
	Llkup->action[__NSW_UNAVAIL] = Action;	/* override */
	Llkup->action[__NSW_TRYAGAIN] = Action;	/* override */
}

/*
 * FFixLookUp:	Final fixup for lookups; the last lookup has
 *	RETURN for all statuses
 */
static  int
FFixLookUp (Flkup)
T_NSW_LOOKUP	*Flkup;
{
	T_NSW_LOOKUP *	tlkup;

	tlkup = Flkup;
	while (tlkup->next != NULL)
		tlkup = tlkup->next;
	FixAActions(tlkup, __NSW_RETURN);
}

/*
 * error_recovery:  emit diagnostic message; recover allocated space;
 *	return error
 */
static  int
error_recovery (es)
char  *es;
{
	int  ii;

	cyerror(es);
	for (ii=0; ii<num_id; ii++)
		free(ar_id[ii]);
	for (ii=0; ii<num_lookup; ii++)
		free_lookup(ar_lookup[ii]);
	reset_vars();
	if (__nsw_debug)
		printf("\n%s.error_recovery: ERR- Error Recovery Completed\n", module);
	return (false);
}


/*----------------------------------------------------------------------
 * Submodule: parser
 *	cyerror()
 *	EmitLex()
 *	#define  skip_empty(CP) \
 *	ident ()
 *	source ()
 *	criteria ()
 *	scan_service_list ()
 *	service_list ()
 *	zyparse()
 *	get_entry()
 ---------------------------------------------------------------------*/

/*
 * cyerror:	emit required message; return error
 */
static  int
cyerror(es)
char  *es;
{
	if (__nsw_debug)
		printf("^%s^", es);
	return (false);
}

/*
 * EmitLex:	print lexical element
 */
static  void
EmitLex (Sym)
char	*Sym;
{
	if (__nsw_debug)
		printf("L<%s>", Sym);
}

#define  skip_empty(CP) \
    while (*CP == ' ' || *CP == '\t' ) \
      CP++

/*
 * ident:	case insensitive/sensitive identifier recognition
 */
#define  CASE_INSENS	0
#define  CASE_SENS	1
static  int
ident (sens)
int	sens;
{
	int	ii;
	char	*chp;

	chp = rlinep;
	skip_empty(chp);

	ii = 0;
	mytext[ii] = '\0';
	while ( isalpha(*chp) )
	{
		mytext[ii] = *chp;
		if (!sens)
			if (isupper(*chp))
				mytext[ii] = tolower(*chp);
		mytext[++ii] = '\0';
		chp++;
	}
	rlinep = chp;
	return ( mytext[0] != '\0');
}

/*
 * source:	scans <source> and augments the lookup structure inh_lkup
 *	(inherited attribute) with the source information.
 */
static  int
source (inh_lkup)
struct __nsw_lookup	*inh_lkup;
{
	if (!ident(CASE_SENS))
		return (false);
	EmitLex(mytext);
	if ((inh_lkup->service_name=alc_id(strlen(mytext)+1)) == NULL) {
		error_recovery("source-id memalloc failed");
		return (false);
	}
	strcpy (inh_lkup->service_name, mytext);
	return (true);
}

/*
 * criteria:	scans <criteria> including the enclosing square brackets and
 *	augments the lookup structure inh_lkup (inherited attribute) with the
 *	criteria information.
 */
static  int
criteria (inh_lkup)
struct __nsw_lookup	*inh_lkup;
{
	char	*chp;
	int	lstatus; /* scanned status */
	int	laction; /* scanned action */

	chp = rlinep;
	skip_empty(chp);

	if (*chp != '[')
		return(error_recovery("Missing ["));
	EmitLex("[");
	rlinep = ++chp;

	while ( (*chp != ']') && (*chp != '\0') )
	{
		lstatus = __NSW_TRASH; /* init lstatus */
		/*
		 * scan <status> and assign to lstatus
		 */
		if (!ident(CASE_INSENS))
			return (error_recovery(chp));
		EmitLex(mytext);
		if (strcmp(mytext, __NSW_STR_SUCCESS) == 0)
			lstatus = __NSW_SUCCESS;
		else if (strcmp(mytext, __NSW_STR_NOTFOUND) == 0)
			lstatus = __NSW_NOTFOUND;
		else if (strcmp(mytext, __NSW_STR_UNAVAIL) == 0)
			lstatus = __NSW_UNAVAIL;
		else if (strcmp(mytext, __NSW_STR_TRYAGAIN) == 0)
			lstatus = __NSW_TRYAGAIN;
		/*
		 * scan '=', advance chp
		 */
		mytext[0] = '\0';
		chp = rlinep;
		skip_empty(chp);
		if (*chp != '=')
			return(error_recovery("Missing ="));
		EmitLex("=");
		rlinep = ++chp;
		skip_empty(chp);
		/*
		 * scan <action>
		 */
		if (!ident(CASE_INSENS))
			return (error_recovery(chp));
		EmitLex(mytext);
		/*
		 * if criterion is Status=Action
		 *	then override default action
		 * if criterion is garbage=X
		 *	then ignore
		 * if criterion is X=garbage
		 *	then error
		 */
		if (strcmp(mytext, __NSW_STR_RETURN) == 0)
			laction = __NSW_RETURN;
		else if (strcmp(mytext, __NSW_STR_CONTINUE) == 0)
			laction = __NSW_CONTINUE;
		else {
			Garbage = true;
			return(error_recovery("[X=garbage]-criterion"));
		}
		if (lstatus != __NSW_TRASH)
			inh_lkup->action[lstatus] = laction;
		/*
		 * init mytext, chp for further scan
		 */
		mytext[0] = '\0';
		chp = rlinep;
		skip_empty(chp);
		rlinep = chp;
	}
	if (*chp != ']')
		return(error_recovery("Missing ]"));
	EmitLex("]");
	rlinep = ++chp;
	return (true);
}

/*
 * scan_service_list:
 *	Scans the list of sources along with the specified criteria
 *	creating and modifying the lookup structure inh_lkup (inherited
 *	attribute) as necessary. Called by service_list().
 */
static  int
scan_service_list (inh_lkup)
struct __nsw_lookup	*inh_lkup;
{
	char	*chp;
	struct  __nsw_lookup	*t_lkup;

	chp = rlinep;
	skip_empty(chp);

	while (*chp != '\0')
	{
		rlinep = chp;
		/*
		 * Allocate a lookup and add to the lookup chain
		 */
		if ((t_lkup=alc_lookup()) == NULL) {
			error_recovery("lookup memalloc falied");
			return (false);
		}
		if (inh_lkup != NULL) {
			struct  __nsw_lookup	*h_lkup;

			for (h_lkup = inh_lkup; h_lkup->next != NULL; h_lkup = h_lkup->next )
				;
			h_lkup->next = t_lkup;
		}
		else 
			st_lkup =
			    inh_lkup = t_lkup;
		/*
		 * scan a service specification - <source> <criteria>
		 */
		if (!source(t_lkup))
			return (error_recovery("Missing <source>"));
		chp = rlinep;
		skip_empty(chp);
		if (*chp != '[')
			continue;
		rlinep = chp;
		if (!criteria(t_lkup))
			return (false);
		chp = rlinep;
		skip_empty(chp);
	}
	return (true);
}

/*
 * service_list:
 *	Parses the list of sources along with the specified criteria
 *	creating and modifying the source lookup structure as necessary.
 *	Handles the possibility of a service list specification which
 *	begins with a criteria (no source).
 */
static  int
service_list ()
{
	char	*chp;
	struct  __nsw_lookup	*t_lkup;

	chp = rlinep;
	skip_empty(chp);
	if (*chp == '[') {
		/*
		 *	handles the possibility of a service list specification
		 *	which begins with a criteria (no source).
		 */
		if (((t_lkup=alc_lookup())==NULL)
		    || ((t_lkup->service_name=alc_id(2)) == NULL)) {
			error_recovery("memalloc failed");
			return (false);
		}
		st_lkup = t_lkup;
		/* No source i.e. source is " " */
		strcpy (t_lkup->service_name, " ");
		rlinep = chp;
		if (!criteria(t_lkup))
			return (false);
		chp = rlinep;
		skip_empty(chp);
		rlinep = chp;
		if (*chp == '\0') {
			/*
			 * do final fix for this lookup
			 * i.e. action for all status is return.
			 */
			FFixLookUp(st_lkup);
			return (true);
		}
	}
	if (*chp == '\0')
		return (error_recovery ("Unexpected End of Input"));
	if (!isalpha(*chp))
		return (error_recovery("Invalid Symbol"));
	rlinep = chp;
	/*
	 * the service_list specification is actually scanned by
	 * scan_service_list()
	 */
	if (!scan_service_list(st_lkup))
		return (false);
	/*
	 * do final fix for the last lookup
	 * i.e. action for all status set to return.
	 */
	FFixLookUp(st_lkup);
	return (true);
}

/*
 * zyparse:    initiates the top-down parse of the string beginning
 *	with the ':' for a database entry
 */
static  int
zyparse ()
{
	char	*chp;

	chp = rlinep;
	skip_empty(chp);

	if (*chp != ':') {
		if (__nsw_debug) {
			cyerror("Missing :");
		}
		return (false);
	}
	EmitLex(":");

	rlinep = ++chp;
	if (!service_list())
		return (false);
	res_lookup = st_lkup;
	return (true);
}


/*
 * get_entry:    gets the database entry in /etc/nsswitch.conf for the
 *	info-class specified as the argument
 */
static  int
get_entry  (entry_id)
char	*entry_id;
{
	char	*chp;
	int	ii;

	while ( !feof(myin) && (fgets(iline, LINSIZ-1, myin) != NULL) )
	{
		mylineno++;
		if (__nsw_debug)
			printf("%s[%s]%2d->%s", module, __NSW_CONF_FILE, mylineno, iline);

		ii = 0;
		mytext[ii] = '\0';
		if ( (chp = strchr(iline, '\n')) != NULL )
			*chp ='\0';
		chp = iline;

		if (   (*chp == '#') || (*chp == '\n') 
                    || (*chp == '\t') || (*chp == ' ') )
			continue;

		while ( isalpha(*chp) )
		{
			mytext[ii++] = *chp;
			mytext[ii] = '\0';
			chp++;
		}

		/*
		 * The switch can be used for other database lookups as
		 * well. Add the databases sources to __nsw_inteqv() and
		 * switch.h.
		 */
		if (!strcmp(entry_id, mytext)==0)
			continue;

		if (__nsw_debug)
			printf("%s[%s]LS->", module, __NSW_CONF_FILE);
		EmitLex(mytext);
		rlinep = chp;
		return (true);
	}
	if (__nsw_debug)
		printf("%s.get_entry: NO %s POLICY ENTRY\n", module, entry_id);
	fclose(myin);
	return (false);
}


/*----------------------------------------------------------------------
 * Submodule:	switch interface routines - these routines constitute the
 *	external interface (visible entry points) provided by the switch and
 *	are for use by switch based modules.
 *	__nsw_inteqv()
 *	__nsw_getconfig()
 *	__nsw_freeconfig()
 *	__nsw_getdefault()
 *	__nsw_dumpconfig()
 ---------------------------------------------------------------------*/

int
__nsw_inteqv __PF1(
src_id, char *
)
{
	char  ec;
	int   ii;

	if ( strcmp(src_id,"dns")==0 )
		return  SRC_DNS;
	else if ( strcmp(src_id, "nis")==0 )
		return  SRC_NIS;
	else if ( strcmp(src_id,"files")==0 )
		return  SRC_FILES;
	return  SRC_UNKNOWN;
}

struct  __nsw_switchconfig *
__nsw_getconfig __PF2(
entry_id, char *,
entry_status, enum  __nsw_parse_err *
)
{
	int	itok;
	T_NSW_SWCONFIG *	nsw_conf_entry=NULL;

	reset_vars();
	if( (myin=fopen(__NSW_CONF_FILE, "r")) == NULL) {
		*entry_status=__NSW_PARSE_NOFILE;
		if (__nsw_debug) {
			fprintf(stderr, "could not open %s\n", __NSW_CONF_FILE);
			printf("__nsw_getconfig: NO POLICY FILE\n");
		}
		return(NULL);
	}
	/*
	 * Note: The code needs to be augmented to check to see if
	 * the file is unreadable/non-ascii type??
	 */

	if (!get_entry(entry_id)) {
		*entry_status=__NSW_PARSE_NOPOLICY;
		return((struct __nsw_switchconfig *)NULL);
	}

	if (zyparse()) {
		*entry_status=__NSW_PARSE_SUCCESS;
		if (__nsw_debug)
			printf("\n__nsw_getconfig: PARSE SUCCESSFUL\n");
		if (res_lookup != NULL) {
			nsw_conf_entry = (T_NSW_SWCONFIG *)malloc(sizeof(T_NSW_SWCONFIG));
			nsw_conf_entry->vers = 1;
			nsw_conf_entry->dbase = (char *)malloc(strlen(entry_id)+1);
			strcpy(nsw_conf_entry->dbase, entry_id);
			nsw_conf_entry->num_lookups = num_lookup;
			nsw_conf_entry->lookups = res_lookup;
		}
		else if (__nsw_debug)
			printf("__nsw_getconfig: ERR- Lookup Structure is Null\n");
		reset_vars();
		return(nsw_conf_entry);
	}
	if (Garbage)
		return(__nsw_getdefault(entry_id));
	*entry_status=__NSW_PARSE_NOPOLICY;
	if (__nsw_debug)
		printf("__nsw_getconfig: ERR- SYNTAX ERROR\n");
	return((struct __nsw_switchconfig *)NULL);
}

int
__nsw_freeconfig __PF1(
nsw_conf, struct  __nsw_switchconfig *
)
{
	T_NSW_LOOKUP	*t_lk;

	while (nsw_conf->lookups != NULL)
	{
		t_lk = nsw_conf->lookups;
		nsw_conf->lookups = t_lk->next;
		free_lookup(t_lk);
	}
	free(nsw_conf);
	return(1);
}

struct  __nsw_switchconfig  *
__nsw_getdefault __PF1(
entry_id, char *
)
{
#define  DEFLKUPS	3
	T_NSW_LOOKUP	*t_lk;
	T_NSW_SWCONFIG *	nsw_conf_entry=NULL;

	reset_vars();
	if (__nsw_debug)
		printf("__nsw_getdefault: default %s lookup policy\n", entry_id);
	if (strcmp(entry_id, __NSW_HOSTS_DB)==0) {
		if ((nsw_conf_entry=(T_NSW_SWCONFIG *) malloc(sizeof(T_NSW_SWCONFIG)))==NULL)
			return (NULL);
		nsw_conf_entry->vers = 1;
		nsw_conf_entry->dbase = (char *)malloc(strlen(__NSW_HOSTS_DB)+1);
		strcpy(nsw_conf_entry->dbase, __NSW_HOSTS_DB);
		nsw_conf_entry->num_lookups = DEFLKUPS;
		nsw_conf_entry->lookups = NULL;
		/* FILES */
		if (((t_lk=alc_lookup())==NULL)
		    || ((t_lk->service_name=alc_id(6))==NULL)) {
			error_recovery("memalloc failed");
			return (NULL);
		}
		FixAActions(t_lk, __NSW_RETURN);
		strcpy(t_lk->service_name, "files");
		t_lk->next = NULL;
		nsw_conf_entry->lookups = t_lk;
		/* NIS */
		if (((t_lk=alc_lookup())==NULL)
		    || ((t_lk->service_name=alc_id(4))==NULL)) {
			error_recovery("memalloc failed");
			return (NULL);
		}
		strcpy(t_lk->service_name, "nis");
		t_lk->next = nsw_conf_entry->lookups;
		nsw_conf_entry->lookups = t_lk;
		/* DNS */
		if (((t_lk=alc_lookup())==NULL)
		    || ((t_lk->service_name=alc_id(4))==NULL)) {
			error_recovery("memalloc failed");
			return (NULL);
		}
		strcpy(t_lk->service_name, "dns");
		t_lk->next = nsw_conf_entry->lookups;
		nsw_conf_entry->lookups = t_lk;
		reset_vars();
		return(nsw_conf_entry);
	}
	if (__nsw_debug)
		printf("__nsw_getdefault: ERR- NO DEFAULT POLICY FOR %s\n", entry_id);
	return (NULL);
}

int
__nsw_dumpconfig __PF1(
nsw_conf, struct  __nsw_switchconfig *
)
{
	int  ii;
	T_NSW_LOOKUP  *tlkup=NULL;

	if (!(__nsw_debug))
		return(-1);
	if (nsw_conf==NULL)
		return(0);
	printf("\t#Lookups = %2d\n", nsw_conf->num_lookups);
	tlkup=nsw_conf->lookups;
	while (tlkup != NULL)
	{
		printf("\t%s [", tlkup->service_name);
		for (ii=0; ii<__NSW_STD_ERRS; ii++)
		{
			switch (__NSW_ACTION(tlkup, ii)) {
			case __NSW_CONTINUE:
				printf("C");
				break;
			case __NSW_RETURN:
				printf("R");
				break;
			default:
				printf("B"); /* __NSW_TRASH */
				break;
			}
		}
		printf("]  ");
		tlkup = tlkup->next;
	}
	printf("\n");
	return (1);
}
