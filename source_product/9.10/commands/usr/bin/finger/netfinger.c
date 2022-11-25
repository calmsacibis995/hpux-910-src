/* HPUX_ID: @(#) $Revision: 66.2 $ */

#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef HP
#define rindex strrchr
#define bcopy(s,d,l) memcpy((d),(s),(l))
#endif

extern int large;

netfinger(name)
	char *name;
{
	char *host;
	char fname[100];
	struct hostent *hp;
	struct servent *sp;
	struct sockaddr_in sin;
	int s;
	char *rindex();
	register FILE *f;
	register int c;
	register int lastc;

	if (name == NULL)
		return (0);
	host = rindex(name, '@');
	if (host == NULL)
		return (0);
	*host++ = 0;
	hp = gethostbyname(host);
	if (hp == NULL) {
		static struct hostent def;
		static struct in_addr defaddr;
		static char *alist[1];
		static char namebuf[128];

		defaddr.s_addr = inet_addr(host);
		if (defaddr.s_addr == -1) {
			printf("unknown host: %s\n", host);
			return (1);
		}
		strcpy(namebuf, host);
		def.h_name = namebuf;
		def.h_addr_list = alist, def.h_addr = (char *)&defaddr;
		def.h_length = sizeof (struct in_addr);
		def.h_addrtype = AF_INET;
		def.h_aliases = 0;
		hp = &def;
	}
	sp = getservbyname("finger", "tcp");
	if (sp == 0) {
		printf("tcp/finger: unknown service\n");
		return (1);
	}
	sin.sin_family = hp->h_addrtype;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = sp->s_port;
	s = socket(hp->h_addrtype, SOCK_STREAM, 0);
	if (s < 0) {
		perror("socket");
		return (1);
	}
	printf("[%s]\n", hp->h_name);
	fflush(stdout);
	if (connect(s, (char *)&sin, sizeof (sin)) < 0) {
		perror("connect");
		close(s);
		return (1);
	}
	if (large) write(s, "/W ", 3);
	write(s, name, strlen(name));
	write(s, "\r\n", 2);
	f = fdopen(s, "r");
	while ((c = getc(f)) != EOF) {
		switch(c) {
		case 0210:
		case 0211:
		case 0212:
		case 0214:
			c -= 0200;
			break;
		case 0215:
			c = '\n';
			break;
		}
		lastc = c;
		if (isprint(c) || isspace(c))
			putchar(c);
		else
			putchar(c ^ 100);
	}
	if (lastc != '\n')
		putchar('\n');
	(void)fclose(f);
	return (1);
}
