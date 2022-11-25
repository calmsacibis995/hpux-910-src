#include "monitor.h"

extern char last_cmd;
extern struct modes *mode;
char help_file[] = "/usr/contrib/lib/monitor.help";
char help_target;

mode_help()
{
	char buf[1024];
	FILE *hfd;

	hfd = fopen(help_file, "r");

	if (hfd == NULL) {
		atf(0, 2, "Can't open help file: %s\n\n", help_file);
		return;
	}

	if (last_cmd != mode->letter) {		/* if it's not us: */
		help_target = last_cmd;		/* it's our subject */
		mode->screen_number = 0;	/* start on first page */
	}

	/* Look for current command */
	while (fgets(buf, sizeof(buf), hfd) > 0) {
		if (buf[0] == '@' && (buf[1] == help_target || buf[1] == '*'))
			break;
	}

	atf(0, 2, "Use 'F' or ' ' to page forward, 'B' or '-' to page backward, [return] to resume\n\n");
	start_scrolling_here();
	while (fgets(buf, sizeof(buf), hfd) > 0) {
		if (buf[0] == '@')			/* Next header? */
			break;
		addf(buf);
	}

	fclose(hfd);
}
