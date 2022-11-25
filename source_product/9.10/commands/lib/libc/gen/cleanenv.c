/*
 * @(#) $Revision: 66.2 $
 */

/*
 * cleanenv.c -- shut off alarm, close file descriptors, trim environment
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define cleanenv   _cleanenv
#define close      _close
#define setitimer  _setitimer
#define strlen     _strlen
#define strncmp    _strncmp
#define memset     _memset
#define getnumfds  _getnumfds
#endif

#include <stdio.h>
#include <varargs.h>
#include <memory.h>
#include <time.h>

#define	reg		register
#define	TIMEVAL		itimerval

static char		PATH[] = "PATH=/bin:/usr/bin";
static char		IFS[] = "IFS= \t\n";	/* space, tab, newline */


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef cleanenv
#pragma _HP_SECONDARY_DEF _cleanenv cleanenv
#define cleanenv _cleanenv
#endif

int
cleanenv(envp, va_alist)
char **envp[];
va_dcl
{
	reg	int		i = 0;
	reg	int		j = 0;

	auto	struct TIMEVAL	itimerval;
	auto	struct TIMEVAL	oitimerval;
	auto	va_list		ap = (va_list)0;
	auto	va_list		sap = (va_list)0;
	auto	char *		bp = (char *)0;
	auto	char **		ev = (char **)0;
	auto	int		evcount = 0;
	auto	int		match = 0;
	auto	int		numfds;


	/*
	 * shut off timers
	 */
	itimerval.it_interval.tv_sec = (unsigned long)0;
	itimerval.it_interval.tv_usec = (long)0;
	itimerval.it_value.tv_sec = (unsigned long)0;
	itimerval.it_value.tv_usec = (long)0;
	(void)setitimer(ITIMER_REAL, &itimerval, &oitimerval);
	(void)setitimer(ITIMER_VIRTUAL, &itimerval, &oitimerval);
	(void)setitimer(ITIMER_PROF, &itimerval, &oitimerval);

	/*
	 * close file descriptors
	 */
	numfds = getnumfds();

	for (i = 3; i < numfds; i++) {
		(void)close(i);
	}

	/*
	 * if envp is 0, don't continue
	 */
	if (envp == (char ***)0) {
		return(0);
	}

	/*
	 * initialize variable args list, and
	 * save vararg pointer for later use
	 */
	va_start(ap);
	sap = ap;

	/*
	 * count number of environment variables
	 * (evcount will be used later)
	 */
	for (evcount = 0; (*envp)[evcount] != (char *)0; evcount++) {
		;
	}

	/*
	 * modify or delete each entry in envp
	 */
	for (ev = *envp; *ev != (char *)0; ev++) {
		/*
		 * change PATH
		 */
		if (strncmp(*ev, "PATH=", 5) == 0) {
			*ev = PATH;
			continue;
		}
		/*
		 * change IFS
		 */
		if (strncmp(*ev, "IFS=", 4) == 0) {
			*ev = IFS;
			continue;
		}
		/*
		 * if not in vararg list, delete it
		 */
		for (ap = sap, match = 0; ; ) {
			/*
			 * get next item off vararg list
			 */
			bp = (char *)va_arg(ap, char *);
			if (bp == (char *)0) {
				break;
			}
			/*
			 * if they match, break out.  match occurs
			 * if environment name is the same, and
			 * it ends in an equals sign.
			 */
			if (strncmp(bp, *ev, strlen(bp)) == 0) {
				if ((*ev)[strlen(bp)] == '=') {
					match = 1;
					break;
				}
			}
		}
		/*
		 * if match fails, zero the string then zero the pointer
		 */
		if (match == 0) {
			(void)memset(*ev, 0, strlen(*ev));
			*ev = (char *)0;
		}
	}

	/*
	 * shift environment up into empty slots
	 */
	for (i = 0; i < evcount; i++) {
		/*
		 * locate empty slot
		 */
		if ((*envp)[i] != (char *)0) {
			continue;
		}
		for (j = i + 1; j < evcount; j++) {
			/*
			 * locate full slot to replace empty slot
			 */
			if ((*envp)[j] != (char *)0) {
				/*
				 * copy full slot then remove it
				 */
				(*envp)[i] = (*envp)[j];
				(*envp)[j] = (char *)0;
				break;
			}
		}
	}
	va_end(ap);
	return(0);
}
