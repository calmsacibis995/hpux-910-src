/* @(#) $Revision: 64.7 $ */    
/*
 * Library routines to convert access control list (ACL) from exact or pattern
 * string (symbolic form) to array of structures.
 *
 * Uses isspace() for NLS portability.
 *
 * POINTER to design documents, technical rationales, etc:  24 documents were
 * placed in shared sources (/hpux/shared/INFO/release_docs/6.5/ACLs/) in 8902.
 */

#ifdef _NAMESPACE_CLEAN
#define strtoacl	_strtoacl
#define strtoaclpatt	_strtoaclpatt
#define aclentrystart	_aclentrystart
#define strcspn		_strcspn
#define getpwnam	_getpwnam
#define strchr		_strchr
#define getgrnam	_getgrnam
#  ifdef __lint
#  define isspace _isspace
#  endif /* __lint */
#endif

#include <unistd.h>		/* for *_OK values */
#include <pwd.h>		/* for getpwnam()  */
#include <grp.h>		/* for getgrnam()  */
#include <string.h>		/* for str*()	   */
#include <ctype.h>		/* for isspace()   */
#include <acllib.h>


/*********************************************************************
 * EXPORTED GLOBAL VALUE:
 *
 * This is exported (not static) so it's available to the caller.  In case of
 * success, it indicates where ACL entries start; in case of error, the first
 * two elements delimit the offending entry.  The "success" case is only useful
 * for strtoaclpatt(), but the array may be altered anyway for strtoacl(),
 * which is harmless.  Note that the code avoids running past the end of this
 * array.
 *
 * The array is initialized so it goes in DATA, not BSS, space, so it can be
 * secondary-def'd.
 */

#define	ATS_MAX	NACLENTRIES	/* allow for pointer to end of last entry */

#ifdef _NAMESPACE_CLEAN
#undef aclentrystart
#pragma _HP_SECONDARY_DEF _aclentrystart aclentrystart
#define aclentrystart _aclentrystart
#endif

char * aclentrystart [ATS_MAX + 1] = { "" };

#define	NOERROR	0		/* for SetEntryStart() */
#define	ERROR	1


/*********************************************************************
 * FORMS OF INPUT / PARSING:
 *
 * acllib.h defines FORM_SHORT and FORM_LONG because they are needed by users.
 * This code also needs FORM_OPERATOR, and insures it is a unique value (as
 * long as the external macros are non-negative).
 */

#define	FORM_OPERATOR  (FORM_SHORT + FORM_LONG + 1)

/*
 * The input may also be in exact or pattern form.  These are called "types",
 * not "forms", to distinguish them.
 */

#define	TYPE_EXACT 0
#define	TYPE_PATT  1


/*********************************************************************
 * MISCELLANEOUS GLOBAL VALUES:
 */

#define	PROC			/* null; easy to find procs */
#define	FALSE	0
#define	TRUE	1
#define	NULL	0
#define	CHNULL	('\0')
#define	CPNULL	((char *) NULL)
#define	REG	register

/*
 * Whitespace skipping may be done redundantly in some places, due to the
 * unfortunate complexity of this code.  (I should have used yacc; sorry.)
 */

#define	SKIPWHITESPACE(cp)  while (isspace (*cp)) cp++

/*
 * These are globals for simplicity and efficiency.
 *
 * cpstart records the start of the current entry separate from aclentrystart[]
 * because if nentries > ATS_MAX, it would not be recorded there.
 *
 * The two different kinds of ACL structure pointers are kept separate rather
 * than passing and casting a (void *).
 */

static	char *cp;				/* current place in string */
static	char *cpstart;				/* for error reporting	   */

static	struct acl_entry	*aclexact;	/* exact form	*/
static	struct acl_entry_patt	*aclpatt;	/* pattern form	*/


/*********************************************************************
 * SPECIAL SYMBOLS:
 */

#define	NONSPECIFIC	'%'		/* special user name / group name  */
#define	FILEID		'@'		/* file owner / group placeholder  */
#define	WILDCARD	'*'		/* match any including NONSPECIFIC */
					/* also used for wildcard modes	   */
/*
 * The END_* macros are chars which terminate a type of symbol, or strings of
 * chars any of which terminates a type of symbol.
 */

#define	OPERATORS	"=+-"		/* mode operator chars		 */

#define	END_USER	"."		/* user name			 */
#define	END_GROUP_OE	OPERATORS	/* group name in operator, exact */
#define	END_GROUP_OP	"=+-,"		/* group name in operator, patt	 */
#define	END_GROUP_SE	","		/* group name in short, exact	 */
#define	END_GROUP_SP	",)"		/* group name in short, pattern */

#define	END_OPERATOR	','		/* end operator entry		    */
#define	START_SHORT	'('		/* start short entry		    */
#define	END_SHORT	')'		/* end short entry; must match term */

#define	OP_EQUAL	'='		/* operators; must match terminators */
#define	OP_PLUS		'+'
#define	OP_MINUS	'-'

#define	MODE_READ	'r'		/* mode chars */
#define	MODE_WRITE	'w'
#define	MODE_EXEC	'x'
#define	MODE_DASH	'-'


/*********************************************************************
 * ERROR RETURN VALUES (must be negative):
 */

#define	ERR_OPENPAREN	(-1)		/* missing START_SHORT		*/
#define	ERR_CLOSEPAREN	(-2)		/* missing END_SHORT		*/
#define	ERR_UNTERMUSER	(-3)		/* unterminated user name	*/
#define	ERR_UNTERMGROUP	(-4)		/* unterminated group name	*/
#define	ERR_NULLUSER	(-5)		/* null user name		*/
#define	ERR_NULLGROUP	(-6)		/* null group name		*/
#define	ERR_INVUSER	(-7)		/* invalid user name		*/
#define	ERR_INVGROUP	(-8)		/* invalid group name		*/
#define	ERR_INVMODE	(-9)		/* invalid mode value (char)	*/
#define	ERR_MAXENTRIES	(-10)		/* would exceed maxentries	*/


/************************************************************************
 * S T R   T O   A C L
 *
 * See the manual entry (strtoacl(3)) for details.
 */
#ifdef _NAMESPACE_CLEAN
#undef strtoacl
#pragma _HP_SECONDARY_DEF _strtoacl strtoacl
#define strtoacl _strtoacl
#endif
PROC int strtoacl (string, nentries, maxentries, acl, fuid, fgid)
	char	*string;		/* to convert			*/
	int	nentries;		/* number in acl[]		*/
	int	maxentries;		/* max size of acl[]		*/
	struct	acl_entry acl[];	/* to receive results		*/
	uid_t	fuid;			/* file owner for FILEID names	*/
	gid_t	fgid;
{
/*
 * CHECK PARAMETERS:
 */

	if (nentries < 0)		/* caller sent negative value */
	    nentries = 0;		/* round it up to minimum     */

	if (nentries > maxentries)	/* already out of bounds */
	{
	    /* overhead for error reporting in this case: */

	    cpstart = ((string == CPNULL) ? "" : string);
	    SetEntryStart (ERROR, 0);

	    return (ERR_MAXENTRIES);
	}

/*
 * DO PARSING:
 */

	aclexact = acl;			/* make global */

	return (ExactOrPatt (TYPE_EXACT, string, nentries, maxentries,
			     fuid, fgid));

} /* strtoacl */


/************************************************************************
 * S T R   T O   A C L   P A T T
 *
 * See the manual entry (strtoacl(3)) for details.
 */
#ifdef _NAMESPACE_CLEAN
#undef strtoaclpatt
#pragma _HP_SECONDARY_DEF _strtoaclpatt strtoaclpatt
#define strtoaclpatt _strtoaclpatt
#endif
PROC int strtoaclpatt (string, maxentries, acl)
	char	*string;		/* to convert		*/
	int	maxentries;		/* max size of acl[]	*/
	struct	acl_entry_patt acl[];	/* to receive results	*/
{
	aclpatt = acl;			/* make global */

	return (ExactOrPatt (TYPE_PATT, string, /* nentries = */ 0, maxentries,
			     /* fuid = */ ACL_FILEOWNER,
			     /* fgid = */ ACL_FILEGROUP));

} /* strtoaclpatt */


/************************************************************************
 * E X A C T   O R   P A T T
 *
 * Given an input type, input string, current and maximum number of entries,
 * file user and group ID values, and globals aclexact and aclpatt, do
 * top-level common handling of either an exact or pattern ACL.  Since this
 * routine handles both operator and short form ACLs, it has minimum possible
 * knowledge about the differences between them.  That is deferred to other
 * routines.
 *
 * Sets global char *cp to the current place in the string and aclentrystart
 * [nentries] to the CHNULL ending the string.
 *
 * Note that all returns come back through this routine.
 */

PROC static ExactOrPatt (type, string, nentries, maxentries, fuid, fgid)
	int	type;			/* exact or pattern		*/
	char	*string;		/* to convert			*/
	int	nentries;		/* number in ACL		*/
	int	maxentries;		/* max size of ACL		*/
	uid_t	fuid;			/* file owner for FILEID names	*/
	gid_t	fgid;
{
/*
 * LOOK FOR NULL ACL STRING POINTER:
 */

	if (string == CPNULL)		/* one kind of null ACL */
	    cp = "";
	else
	{
	    cp = string;
	    SKIPWHITESPACE (cp);	/* now it's safe */
	}

/*
 * HANDLE NULL, OPERATOR, OR SHORT ACL:
 *
 * Doing the handling of both flavors with common code might be smaller but it
 * would be less efficient and harder to understand and maintain.
 */

	if ((*cp == CHNULL)			/* either kind of null ACL */
	 || ((nentries = ((*cp == START_SHORT) ?
		    ShortString    (type, nentries, maxentries, fuid, fgid) :
		    OperatorString (type, nentries, maxentries, fuid, fgid)))
	     >= 0))
	{
	    SetEntryStart (NOERROR, nentries);	/* point to end of string */
	}
	else
	{
	    SetEntryStart (ERROR, 0);		/* for error reporting */
	}

	return (nentries);

} /* ExactOrPatt */


/************************************************************************
 * O P E R A T O R   S T R I N G
 *
 * Given an input type, current and maximum number of entries, file user and
 * group ID values, and global char *cp pointing to the start of stuff
 * (username) in the first entry, parse a (non-null) operator form ACL string.
 * Return values are as for strtoacl().  Set global aclentrystart [nentries] at
 * the start of each entry.
 *
 * Summary of syntax at this level:
 *
 *	stuff [, stuff...]
 *
 * Stuff might start, contain, and/or end with whitespace.
 */

PROC static int OperatorString (type, nentries, maxentries, fuid, fgid)
	int	type;			/* exact or pattern		*/
REG	int	nentries;		/* number of entries in ACL	*/
	int	maxentries;		/* maximum allowed in ACL	*/
	uid_t	fuid;			/* file owner for FILEID names	*/
	gid_t	fgid;
{
/*
 * If OneEntry() succeeds, it leaves cp at the END_OPERATOR or CHNULL past the
 * entry.  If there is an END_OPERATOR, eat it and restart the loop with cp at
 * the start of the next stuff (username) in the next entry, else return.
 */

	while (TRUE)				/* until return */
	{
	    SetEntryStart (NOERROR, nentries);

	    if (((nentries =
		  OneEntry (type, FORM_OPERATOR, nentries, maxentries,
			    fuid, fgid)) < 0)	/* error of some kind	  */
	     || (*cp == CHNULL))		/* expected end of string */
	    {
		return (nentries);
	    }

	    cp++;				/* skip END_OPERATOR */
	}

} /* OperatorString */


/************************************************************************
 * S H O R T   S T R I N G
 *
 * Given an input type, current and maximum number of entries, file user and
 * group ID values, and global char *cp pointing to START_SHORT in the first
 * entry, parse a (non-null) short form ACL string.  Return values are as for
 * strtoacl().  Set global aclentrystart [nentries] at the start of each entry.
 *
 * Summary of syntax at this level:
 *
 *	( stuff ) [( stuff...]
 *
 * Stuff might start, contain, and/or end with whitespace.
 */

PROC static int ShortString (type, nentries, maxentries, fuid, fgid)
	int	type;			/* exact or pattern		*/
REG	int	nentries;		/* number of entries in ACL	*/
	int	maxentries;		/* maximum allowed in ACL	*/
	uid_t	fuid;			/* file owner for FILEID names	*/
	gid_t	fgid;
{
/*
 * HANDLE EACH ENTRY:
 *
 * If OneEntry() succeeds, it leaves cp at the END_SHORT past the entry.
 */

	while (TRUE)				/* until return */
	{
	    SetEntryStart (NOERROR, nentries);

	    if (*cp++ != START_SHORT)		/* never for first entry */
		return (ERR_OPENPAREN);		/* unexpected character  */

	    if ((nentries =
		 OneEntry (type, FORM_SHORT, nentries, maxentries, fuid, fgid))
		< 0)				/* error of some kind */
	    {
		return (nentries);
	    }

/*
 * CHECK FOR NEXT ENTRY:
 *
 * Eat the END_SHORT and any following whitespace, then check for end of
 * string.
 */

	    if (*cp == CHNULL)			/* missing END_SHORT */
		return (ERR_CLOSEPAREN);

	    cp++;				/* skip END_SHORT */
	    SKIPWHITESPACE (cp);

	    if (*cp == CHNULL)			/* expected end of string */
		return (nentries);

	} /* while */

} /* ShortString */


/************************************************************************
 * O N E   E N T R Y
 *
 * Given an input type, an input form, current and maximum number of entries,
 * file user and group ID values, and global char *cp pointing to the start of
 * the user name in the next entry (possibly with leading whitespace), parse
 * one entry, update cp to point to the END_OPERATOR or END_SHORT (as
 * appropriate) or the CHNULL at the end of the entry, and return the new
 * number of entries (> 0) or an error (< 0).
 *
 * Summary of syntax at this level:
 *
 *	operator form:	username . groupname <=|+|-> mode <,|CHNULL>
 *	short form:	username . groupname ,	     mode <)|CHNULL>
 *
 * For patterns, the operator/comma and mode parts are optional.
 *
 * Since this routine handles both operator and short entries, it has minimum
 * possible knowledge about the differences between them.  That is deferred to
 * other routines.
 */

PROC static int OneEntry (type, form, nentries, maxentries, fuid, fgid)
	int	type;			/* exact or pattern		*/
	int	form;			/* form of ACL input		*/
	int	nentries;		/* number in ACL		*/
	int	maxentries;		/* max size of ACL		*/
	uid_t	fuid;			/* file owner for FILEID names	*/
	gid_t	fgid;
{
	uid_t	cuid;			/* current values, may be < 0	*/
	gid_t	cgid;
REG	int	entry;			/* selected entry in ACL	*/
	int	result;			/* from mode parsing		*/

/*
 * GET USER AND GROUP NAMES:
 */

	if ((cuid = NameToID (type, form, /* user = */ TRUE, fuid)) < 0)
	    return (cuid);		/* error value */

	cp++;				/* skip END_USER separator */

	if ((cgid = NameToID (type, form, /* user = */ FALSE, fgid)) < 0)
	    return (cgid);		/* error value */

	/* cp is now at a legal char following the group name */

/*
 * FIND OR ADD MATCHING ENTRY:
 */

	entry = (type == TYPE_EXACT) ? FindEntry (nentries, cuid, cgid)
				     : nentries;	/* always add */

	/* now entry is the existing entry, or the end if need to add */

	if (entry == nentries)				/* add new at end */
	{
	    if (++nentries > maxentries)		/* can't add one */
		return (ERR_MAXENTRIES);

	    if (type == TYPE_EXACT)
	    {
		aclexact [entry] . uid  = (aclid_t) cuid;
		aclexact [entry] . gid  = (aclid_t) cgid;
		aclexact [entry] . mode = 0;	/* initially null access */
	    }
	    else
	    {
		aclpatt [entry] . uid = (aclid_t) cuid;
		aclpatt [entry] . gid = (aclid_t) cgid;
		aclpatt [entry] . onmode  = 0;	/* initially null bits */
		aclpatt [entry] . offmode = 0;
	    }
	}

/*
 * SET OR MODIFY ENTRY'S MODE VALUE:
 */

	if ((result = ((form == FORM_OPERATOR) ? OperatorMode (type, entry)
					       : ShortMode (type, entry))) < 0)
	{
	    return (result);
	}

	return (nentries);

} /* OneEntry */


/************************************************************************
 * N A M E   T O   I D
 *
 * Given an input type, an input form, an ID type flag (user or group), a file
 * ID value to use for FILEID names, and global char *cp set to the start of
 * the user or group name (possibly with leading or trailing whitespace),
 * convert the name to a numeric value, including recognizing NONSPECIFIC,
 * FILEID, possibly WILDCARD (patterns only), and numeric values if name is not
 * found in the password or group file.  Return an ID value (>= 0) if name is
 * decipherable, else an error value (< 0).
 *
 * If a user or group name is not terminated in the current entry but there is
 * a terminator later in the string (e.g.  in a following entry), this routine
 * dumbly returns ERR_INV* instead of ERR_UNTERM*.
 *
 * If successful, leave cp set at the terminator char after the name, which
 * varies depending on the input type and form.
 *
 * Since this routine handles IDs in both operator and short form entries, it
 * has minimum possible knowledge about the differences between them.
 *
 * It might be more efficient if this routine cached values already seen, but
 * in ACLs repetition of the same uid or gid should be the exception, not the
 * norm, and the whole password or group file is likely to be cached by the
 * system anyway.
 */

PROC static int NameToID (type, form, isuser, fid)
	int	type;			/* exact or pattern	*/
	int	form;			/* form of ACL input	*/
REG	int	isuser;			/* flag:  user/group	*/
	int	fid;			/* file ID value	*/
{
	char	*cpend;			/* end of name		*/
	char	*cpnext;		/* next token past name	*/
	char	chsave;			/* saved input char	*/
	struct	passwd *pwd;		/* from getpwnam()	*/
	struct	group  *grp;		/* from getgrnam()	*/
	int	badname = FALSE;	/* temporary flag	*/
	int	result;			/* to return		*/

/*
 * FIND NEXT TOKEN AFTER NAME:
 *
 * User name is followed by END_USER; group name by one of the END_GROUP_*
 * forms, depending on type and form.  Note that a null name can and must be
 * detected BEFORE trimming any trailing whitespace (from a non-null name).
 */

	SKIPWHITESPACE (cp);		/* if any before name */

	cpnext = cp + strcspn (cp, isuser ? END_USER :
		(form == FORM_OPERATOR) ?
		    ((type == TYPE_EXACT) ? END_GROUP_OE : END_GROUP_OP) :
		    ((type == TYPE_EXACT) ? END_GROUP_SE : END_GROUP_SP));

	/* now cpnext points to the next token or to CHNULL */

	if ((*cpnext == CHNULL)			/* no next token in string */
	 && (isuser || (type != TYPE_PATT)))	/* supposed to be one	   */
	{
	    return (isuser ? ERR_UNTERMUSER : ERR_UNTERMGROUP);
	}

	if (cp == cpnext)			/* null name */
	    return (isuser ? ERR_NULLUSER : ERR_NULLGROUP);

/*
 * FIND AND MARK END OF NAME:
 */

	cpend = cpnext;

	while (isspace (cpend [-1]))	/* previous char is whitespace	*/
	    cpend--;			/* back up to it		*/

	/*
	 * Now cpend points to the separator char or first whitespace char
	 * (if any) after the name.
	 */

	chsave = *cpend;		/* temporarily truncate name	*/
	*cpend = CHNULL;		/* fix string before returning!	*/

/*
 * CHECK SPECIAL NAME SYMBOLS:
 */

	if ((cp [0] == NONSPECIFIC) && (cp [1] == CHNULL))
	{					/* is a non-specific value */
	    result = isuser ? ACL_NSUSER : ACL_NSGROUP;
	}
	else if ((cp [0] == FILEID) && (cp [1] == CHNULL))
	{					/* is file owner value */
	    result = fid;
	}
	else if ((type == TYPE_PATT)
	      && (cp [0] == WILDCARD) && (cp [1] == CHNULL))
	{					/* is wildcard value */
	    result = isuser ? ACL_ANYUSER : ACL_ANYGROUP;
	}

/*
 * CHECK KNOWN NAMES AND NUMERIC VALUES:
 */

	else if (isuser ? ((pwd = getpwnam (cp)) != (struct passwd *) NULL)
			: ((grp = getgrnam (cp)) != (struct group  *) NULL))
	{
	    /*
	     * It appears getpwnam() and getgrnam() never leave a file open,
	     * so there is no need to call endpwent() or endgrent().
	     */

	    result = isuser ? (pwd -> pw_uid) : (grp -> gr_gid);

	    /* a negative value would appear to be an error; that's OK */
	}
	else					/* try it as a number */
	{
	    badname = ((result = StrToNum (cp)) < 0);
	}

/*
 * FINISH UP:
 */

	*cpend = chsave;		/* unmodify caller's string */
	cp     = cpnext;		/* advance to terminator    */

	if (badname)
	    return (isuser ? ERR_INVUSER : ERR_INVGROUP);

	return (result);

} /* NameToID */


/************************************************************************
 * S T R   T O   N U M
 *
 * Given a pointer to a null-terminated string, return the numeric equivalent,
 * base-10, of the digits in the string (>= 0) or -1 if the string contains
 * anything other than a digit.  Doesn't handle integer overflow; it might look
 * like an error return (but that's OK).
 *
 * I was unable to find (exactly) this routine in standard libraries.
 */

PROC static int StrToNum (string)
REG	char	*string;
{
REG	int	result = 0;		/* to return */

	while (*string != CHNULL)
	{
	    if ((*string < '0') || (*string > '9'))
		return (-1);

	    result = (result * 10) + ((*string++) - '0');
	}

	return (result);

} /* StrToNum */


/************************************************************************
 * F I N D   E N T R Y
 *
 * Given the current number of entries in an exact ACL and user and group ID
 * values, look for the specified entry in global aclexact.  If found, return
 * its index; otherwise, return nentries (new index).  In any case, never
 * modify the ACL itself (leave that to the caller); a new index may be beyond
 * the limit.
 */

PROC static int FindEntry (nentries, uid, gid)
REG	int	nentries;	/* number in ACL    */
REG	uid_t	uid;		/* which entry	    */
REG	gid_t	gid;
{
REG	int	entry;		/* current element */

	for (entry = 0; entry < nentries; entry++)
	{
	    if ((uid == (uid_t) (aclexact [entry] . uid))
	     && (gid == (gid_t) (aclexact [entry] . gid)))
	    {
		break;
	    }
	}

	return (entry);

} /* FindEntry */


/************************************************************************
 * O P E R A T O R   M O D E
 *
 * Given an input type, an entry index, and global char *cp which points to the
 * first operator in a mode string (or for patterns if no mode string, the
 * following END_OPERATOR or CHNULL), apply the operator(s) and mode
 * character(s) to the mode or onmode and offmode values in the given entry.
 * Ignore whitespace.  If successful, return 0 and leave cp set to the
 * terminating END_OPERATOR or CHNULL.  Otherwise, return ERR_INVMODE.
 *
 * Summary of syntax at this level:
 *
 *	<=|+|-> mode [<=|+|-> mode]... <,|CHNULL>
 */

PROC static int OperatorMode (type, entry)
REG	int	type;		/* exact or pattern	*/
	int	entry;		/* which entry to alter	*/
{
/*
 * CHECK FOR ABSENT MODE:
 */

	if ((type == TYPE_PATT) && ((*cp == END_OPERATOR) || (*cp == CHNULL)))
	    return (0);

/*
 * PARSE NEXT OPERATOR AND MODE VALUE, CHECK FOR END:
 *
 * Don't skip dashes because they mean minus operators.
 */

	while (TRUE)			/* until return */
	{
	    ModeToNum (type, *cp++, entry, /* skipdash = */ FALSE);
	    SKIPWHITESPACE (cp);

	    if ((*cp == END_OPERATOR) || (*cp == CHNULL))  /* end of mode */
		return (0);

	    if (strchr (OPERATORS, *cp) == CPNULL)	/* invalid char	*/
		return (ERR_INVMODE);

	}

} /* OperatorMode */


/************************************************************************
 * S H O R T   M O D E
 *
 * Given an input type, an entry index, and global char *cp which points to the
 * separator char before a mode string (or for patterns if no mode string, the
 * following END_SHORT or CHNULL), set the mode value in the entry based on the
 * mode character(s).  Ignore whitespace and MODE_DASH.  If successful, return
 * 0 and leave cp set to the terminating END_SHORT.  Otherwise, if the next
 * char is CHNULL or START_SHORT (a special case), return ERR_CLOSEPAREN, else
 * return ERR_INVMODE.
 *
 * Summary of syntax at this level:
 *
 *	, mode <)|CHNULL>
 */

PROC static int ShortMode (type, entry)
	int	type;		/* exact or pattern	*/
	int	entry;		/* which entry to alter	*/
{
	if ((type == TYPE_PATT) && (*cp == END_SHORT))	 /* absent mode */
	    return (0);

	cp++;					/* skip END_GROUP_SE */

	ModeToNum (type, OP_EQUAL, entry, /* skipdash = */ TRUE);
	SKIPWHITESPACE (cp);

	return ( (*cp == END_SHORT) ? 0 :
		((*cp == CHNULL) || (*cp == START_SHORT)) ? ERR_CLOSEPAREN :
		ERR_INVMODE);

} /* ShortMode */


/************************************************************************
 * M O D E   T O   N U M
 *
 * Given an input type, an operator in effect, an entry index, a flag whether
 * to skip dashes, global char *cp which points to the first char of a mode
 * string (in operator form input, the mode part of an <op mode> construct),
 * and globals aclexact and aclpatt, parse the string and alter mode (or onmode
 * and offmode) accordingly using *_OK bit values.  Ignore whitespace chars and
 * possibly dashes in the string.  Leave cp set to the first unexpected char
 * found, including possibly whitespace in some cases.
 *
 * Summary of syntax at this level:
 *
 *	[<0..7> | <r|w|x|->... | * ] <anything else>
 *
 * WILDCARD ("*") is allowed for patterns only.  With OP_EQUAL, it causes no
 * changes to onmode or offmode.  Otherwise, it sets all mode bits in one mask
 * or the other, as appropriate for the operator.
 *
 * Note that only the bits in MODEMASK can be set in mode, onmode, or offmode.
 */

PROC static ModeToNum (type, op, entry, skipdashes)
	int	type;		/* exact or pattern	*/
	int	op;		/* OP_* value		*/
	int	entry;		/* which array element	*/
	int	skipdashes;	/* flag			*/
{
REG	int	mode = 0;	/* mode value to apply	*/

/*
 * HANDLE WILDCARD MODE:
 */

	SKIPWHITESPACE (cp);

	if ((type == TYPE_PATT) && (*cp == WILDCARD))
	{
	    cp++;				/* consume one char only */

	    if (op == OP_EQUAL)
		return;				/* no changes needed */

	    mode = 0xFFFFFFFF & MODEMASK;	/* set all valid mode bits */
	    goto SetMode;
	}

/*
 * HANDLE OCTAL MODE:
 */

	if ((*cp >= '0') && (*cp <= '7'))	/* octal form */
	{
	    mode = (*cp++) - '0';		/* consume one char only */
	    goto SetMode;
	}

/*
 * HANDLE SPECIFIC MODE CHARS:
 */

	while (TRUE)			/* until goto */
	{
	    switch (*cp)
	    {
	    case MODE_READ:	mode |= R_OK;	break;
	    case MODE_WRITE:	mode |= W_OK;	break;
	    case MODE_EXEC:	mode |= X_OK;	break;
	    case MODE_DASH:	if (skipdashes)	break;	/* else fall through */
	    default:		goto SetMode;		/* == "break; break" */
	    }

	    cp++;
	    SKIPWHITESPACE (cp);
	}

/*
 * SET MODE VALUE (IN EXACT ACL):
 */

SetMode:	/* come here with mode set */

	if (type == TYPE_EXACT)
	{
	    REG struct acl_entry *entryp = aclexact + entry;

	    if	    (op == OP_EQUAL)	(entryp -> mode)  =    mode;
	    else if (op == OP_PLUS)	(entryp -> mode) |=    mode;
	    else	/* OP_MINUS */	(entryp -> mode) &= (~ mode);
	}

/*
 * ALTER ONMODE AND OFFMODE (IN PATTERN):
 *
 * Turn on AND off the indicated bits, depending on the operator.
 */

	else
	{
	    REG struct acl_entry_patt *entryp = aclpatt + entry;

	    if (op == OP_EQUAL)
	    {
		(entryp -> onmode)  =    mode;
		(entryp -> offmode) = (~ mode) & MODEMASK;
	    }
	    else if (op == OP_PLUS)
	    {
		(entryp -> onmode)  |=	  mode;
		(entryp -> offmode) &= (~ mode);
	    }
	    else /* OP_MINUS */
	    {
		(entryp -> onmode)  &= (~ mode); 
		(entryp -> offmode) |=	  mode;
	    }
	}

} /* ModeToNum */


/************************************************************************
 * S E T   E N T R Y   S T A R T
 *
 * Given:
 *
 * - error flag indicating whether to handle normal parsing or an error;
 * - entry index, for the normal case;
 * - global char *cp pointing to the current place in the entry string, for
 *   the normal case;
 * - global char *cpstart pointing to the start of the current entry (possibly
 *   with leading whitespace), for the error case;
 * - global char *aclentrystart[];
 *
 * set value(s) in aclentrystart[].  In the normal case, save cp in the current
 * entry index if not out of bounds, and set cpstart in case of later error.
 * In the error case, ignore the entry index; advance cpstart to the first
 * non-whitespace character or CHNULL, then set aclentrystart[0] there and
 * aclentrystart[1] (and cpstart) to the start of the next entry.
 *
 * If the first non-whitespace char is not START_SHORT, assume operator form
 * and look for the next END_OPERATOR or CHNULL.  Otherwise, assume short form
 * and look for END_SHORT (and move past it) or CHNULL.  Note that the form of
 * the current entry, not the form of the whole ACL string, determines the
 * terminator char.
 */

PROC static SetEntryStart (errflag, nentries)
	int	errflag;
	int	nentries;
{
REG	int	chterm;			/* desired terminator char */

/*
 * NORMAL CASE:
 */

	if (errflag == NOERROR)
	{
	    if (nentries <= ATS_MAX)		/* not out of bounds */
		aclentrystart [nentries] = cp;

	    cpstart = cp;
	    return;
	}

/*
 * ERROR CASE:
 */

	SKIPWHITESPACE (cpstart);

	chterm = (* (aclentrystart [0] = cpstart) == START_SHORT) ?
		 END_SHORT : END_OPERATOR;

	while ((*cpstart != chterm) && (*cpstart != CHNULL))
	    cpstart++;

	if (*cpstart == END_SHORT)
	    cpstart++;			/* skip closing paren */

	aclentrystart [1] = cpstart;

} /* SetEntryStart */
