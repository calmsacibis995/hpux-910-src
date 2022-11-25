/*
 * @(#) $Revision: 64.2 $
 */

/*
 * Function Name:	show_audit
 *
 * Abstract:		Print out audit status.
 *                      Uses NLS message numbers 51-60.
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

extern int		errno;

int
show_audit()
{
	reg	int		i = 0;

	auto	struct SAR	sar;
	auto	int		rvalue = 0;
	auto	char *		function = "show_audit";


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
			catgets(nlmsg_fd,NL_SETN,51,"audwrite(%s) failed: %s\n"),sar.aud_body.text,_perror());
		return(-1);
	}

	/*
	 * check return status of getevent()
	 */
	if (rvalue != 0) {
		(void)error(function,
			catgets(nlmsg_fd,NL_SETN,52,"getevent() failed: %s\n"),_perror());
		return(-1);
	}

	/*
	 * show event status
	 */
	for (i = 0; i < EVMAPSIZE; i++) {
		if (ev_map[i].ev_mark) {
			if (show_event(ev_map[i].ev_name) != 0) {
				(void)error(function,
					catgets(nlmsg_fd,NL_SETN,53,"show_event(%s) failed\n"),ev_map[i].ev_name);
				return(-1);
			}
		}
	}

	/*
	 * show syscall status
	 */
	for (i = 0; i < SCMAPSIZE; i++) {
		if (sc_map[i].sc_mark) {
			show_status(
				"syscall",
				sc_map[i].sc_name,
				sc_tab[sc_map[i].sc_index].logit);
		}
	}
	return(0);
}

/*
 * this is a hack.  it'll get straightened out
 * once the event structure becomes an array.
 */
int
show_event(name)
char *name;
{
	if (strcmp(name, "create") == 0) {
		show_status("event", "create", ev_tab.create);
	}
	else if (strcmp(name, "delete") == 0) {
		show_status("event", "delete", ev_tab.delete);
	}
	else if (strcmp(name, "moddac") == 0) {
		show_status("event", "moddac", ev_tab.moddac);
	}
	else if (strcmp(name, "modaccess") == 0) {
		show_status("event", "modaccess", ev_tab.modaccess);
	}
	else if (strcmp(name, "open") == 0) {
		show_status("event", "open", ev_tab.open);
	}
	else if (strcmp(name, "close") == 0) {
		show_status("event", "close", ev_tab.close);
	}
	else if (strcmp(name, "process") == 0) {
		show_status("event", "process", ev_tab.process);
	}
	else if (strcmp(name, "removable") == 0) {
		show_status("event", "removable", ev_tab.removable);
	}
	else if (strcmp(name, "login") == 0) {
		show_status("event", "login", ev_tab.login);
	}
	else if (strcmp(name, "admin") == 0) {
		show_status("event", "admin", ev_tab.admin);
	}
	else if (strcmp(name, "ipccreat") == 0) {
		show_status("event", "ipccreat", ev_tab.ipccreat);
	}
	else if (strcmp(name, "ipcopen") == 0) {
		show_status("event", "ipcopen", ev_tab.ipcopen);
	}
	else if (strcmp(name, "ipcclose") == 0) {
		show_status("event", "ipcclose", ev_tab.ipcclose);
	}
	else if (strcmp(name, "uevent1") == 0) {
		show_status("event", "uevent1", ev_tab.uevent1);
	}
	else if (strcmp(name, "uevent2") == 0) {
		show_status("event", "uevent2", ev_tab.uevent2);
	}
	else if (strcmp(name, "uevent3") == 0) {
		show_status("event", "uevent3", ev_tab.uevent3);
	}
	else if (strcmp(name, "ipcdgram") == 0) {
		show_status("event", "ipcdgram", ev_tab.ipcdgram);
	}
	else {
		return(-1);
	}
	return(0);
}
