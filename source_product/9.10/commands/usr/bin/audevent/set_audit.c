/*
 * @(#) $Revision: 64.3 $
 */

/*
 * Function Name:	set_audit
 *
 * Abstract:		For each marked event and system call,
 *			set audit bit for success and/or failure.
 *                      Uses NLS message numbers 41-50.
 *
 * Return Value:	returns 0 on success, -1 on failure
 */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
extern nl_catd nlmsg_fd;
#endif NLS

#include <sys/types.h>
#include <sys/audit.h>
#include "define.h"
#include "extern.h"

#define	SAR		self_audit_rec
#define	reg		register

#define	SETPASS(x)	if (pass != -1) {x = pass ? (x | PASS) : (x & ~PASS);}
#define	SETFAIL(x)	if (fail != -1) {x = fail ? (x | FAIL) : (x & ~FAIL);}
#define	SETBITS(x)	SETPASS(x); SETFAIL(x);

extern int		set_syscall();
extern int		errno;

int
set_audit()
{
	reg	int		i = 0;

	auto	struct SAR	sar;
	auto	int		rvalue = 0;
	auto	char *		function = "set_audit";


	/*
	 * get event and syscall tables
	 */
	rvalue = getevent(sc_tab, &ev_tab);

	/*
	 * audit getevent()
	 */
	(void)sprintf(sar.aud_body.text,
		"audevent: getting event and syscall status");
	sar.aud_head.ah_len = strlen(sar.aud_body.text);
	sar.aud_head.ah_error = rvalue == 0 ? 0 : errno;
	sar.aud_head.ah_event = EN_AUDEVENT;
	if (audwrite(&sar) != 0) {
		(void)error(function,
			catgets(nlmsg_fd,NL_SETN,41,"audwrite(%s) failed: %s\n"),sar.aud_body.text,_perror());
		return(-1);
	}

	/*
	 * check return status of getevent()
	 */
	if (rvalue != 0) {
		(void)error(function,
			catgets(nlmsg_fd,NL_SETN,42,"getevent() failed: %s\n"),_perror());
		return(-1);
	}

	/*
	 * set each event, if marked, and the associated system calls
	 */
	for (i = 0; i < EVMAPSIZE; i++) {
		if (ev_map[i].ev_mark) {
			if (set_event(ev_map[i].ev_name) != 0) {
				(void)error(function,
					catgets(nlmsg_fd,NL_SETN,43,"set_event(%s) failed\n"),ev_map[i].ev_name);
				return(-1);
			}
			if (set_syscall(ev_map[i].ev_type) != 0) {
				(void)error(function,
					catgets(nlmsg_fd,NL_SETN,44,"set_syscall(%d) failed\n"),ev_map[i].ev_type);
				return(-1);
			}
		}
	}

	/*
	 * set syscalls.
	 *
	 * we could use set_syscall() here,
	 * but that would be wasteful.
	 *
	 */
	for (i = 0; i < SCMAPSIZE; i++) {
		if (sc_map[i].sc_mark) {
			SETBITS(sc_tab[sc_map[i].sc_index].logit);
		}
	}

	/*
	 * set status
	 */
	rvalue = setevent(sc_tab, &ev_tab);

	/*
	 * audit setevent()
	 */
	sar.aud_head.ah_error = rvalue == 0 ? 0 : errno;
	sar.aud_head.ah_event = EN_AUDEVENT;
	for (i = 0; i < EVMAPSIZE; i++) {
		if (ev_map[i].ev_mark) {
			sar.aud_body.text[0] = (char)0;
			if (pass != -1) {
				(void)sprintf(sar.aud_body.text,
					"\naudevent: %s %s for %s %s",
					pass == 0 ? "disable" : "enable",
					"success",
					"event",
					ev_map[i].ev_name);
			}
			if (fail != -1) {
				(void)sprintf(sar.aud_body.text +
				    strlen(sar.aud_body.text),
					"\naudevent: %s %s for %s %s",
					fail == 0 ? "disable" : "enable",
					"failure",
					"event",
					ev_map[i].ev_name);
			}
			sar.aud_head.ah_len = strlen(sar.aud_body.text);
			if (audwrite(&sar) != 0) {
				(void)error(function,
					catgets(nlmsg_fd,NL_SETN,45,"audwrite(%s) failed: %s\n"),sar.aud_body.text,_perror());
				return(-1);
			}
		}
	}
	for (i = 0; i < SCMAPSIZE; i++) {
		if (sc_map[i].sc_mark) {
			sar.aud_body.text[0] = (char)0;
			if (pass != -1) {
				(void)sprintf(sar.aud_body.text,
					"\naudevent: %s %s for %s %s",
					pass == 0 ? "disable" : "enable",
					"success",
					"syscall",
					sc_map[i].sc_name);
			}
			if (fail != -1) {
				(void)sprintf(sar.aud_body.text +
				    strlen(sar.aud_body.text),
					"\naudevent: %s %s for %s %s",
					fail == 0 ? "disable" : "enable",
					"failure",
					"syscall",
					sc_map[i].sc_name);
			}
			sar.aud_head.ah_len = strlen(sar.aud_body.text);
			if (audwrite(&sar) != 0) {
				(void)error(function,
					catgets(nlmsg_fd,NL_SETN,45,"audwrite(%s) failed: %s\n"),sar.aud_body.text,_perror());
				return(-1);
			}
		}
	}

	/*
	 * check return value of setevent()
	 */
	if (rvalue != 0) {
                (void)error(function,
                        catgets(nlmsg_fd,NL_SETN,46,"setevent() failed: %s\n"),_perror());
                return(-1);
        }

	return(0);
}

/*
 * this is a hack.  it will change if/when the event
 * table changes from a structure to an array.
 */
int
set_event(name)
char *name;
{
	if (strcmp(name, "create") == 0) {
		SETBITS(ev_tab.create);
	}
	else if (strcmp(name, "delete") == 0) {
		SETBITS(ev_tab.delete);
	}
	else if (strcmp(name, "moddac") == 0) {
		SETBITS(ev_tab.moddac);
	}
	else if (strcmp(name, "modaccess") == 0) {
		SETBITS(ev_tab.modaccess);
	}
	else if (strcmp(name, "open") == 0) {
		SETBITS(ev_tab.open);
	}
	else if (strcmp(name, "close") == 0) {
		SETBITS(ev_tab.close);
	}
	else if (strcmp(name, "process") == 0) {
		SETBITS(ev_tab.process);
	}
	else if (strcmp(name, "removable") == 0) {
		SETBITS(ev_tab.removable);
	}
	else if (strcmp(name, "login") == 0) {
		SETBITS(ev_tab.login);
	}
	else if (strcmp(name, "admin") == 0) {
		SETBITS(ev_tab.admin);
	}
	else if (strcmp(name, "ipccreat") == 0) {
		SETBITS(ev_tab.ipccreat);
	}
	else if (strcmp(name, "ipcopen") == 0) {
		SETBITS(ev_tab.ipcopen);
	}
	else if (strcmp(name, "ipcclose") == 0) {
		SETBITS(ev_tab.ipcclose);
	}
	else if (strcmp(name, "uevent1") == 0) {
		SETBITS(ev_tab.uevent1);
	}
	else if (strcmp(name, "uevent2") == 0) {
		SETBITS(ev_tab.uevent2);
	}
	else if (strcmp(name, "uevent3") == 0) {
		SETBITS(ev_tab.uevent3);
	}
	else if (strcmp(name, "ipcdgram") == 0) {
		SETBITS(ev_tab.ipcdgram);
	}
	else {
		return(-1);
	}
	return(0);
}

int
set_syscall(evid)
int evid;
{
	reg	int		i = 0;


	for (i = 0; i < SCMAPSIZE; i++) {
		if (evid == sc_map[i].sc_type) {
			SETBITS(sc_tab[sc_map[i].sc_index].logit);
		}
	}
	return(0);
}
