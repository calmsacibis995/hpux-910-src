#ifdef	INSTRUMENT

#define	_HPUX_SOURCE
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>

#define INST_UDP_PORT		42961		/* any unused port */
#define INST_UDP_ADDR		0x0f01781e	/* 15.1.120.30 - hpmonk */

static struct
{
	int pid;
	unsigned long stack;
	int mode;
} packet;

static struct sockaddr_in address, myaddress;

void send_packet (unsigned long stack, int mode)
{
	int s = socket(AF_INET,SOCK_DGRAM,0);

#if	DEBUG & 128
	switch (mode)
	{
		case 0:
			write(2,"old\n",4);
			break;
		case 1:
			write(2,"new\n",4);
			break;
		case 2:
			write(2,"rts\n",4);
			break;
		default:
			write(2,"none\n",5);
			break;
	}
#endif
	/* initialize data */
	packet.pid = getpid();
	packet.stack = stack;
	packet.mode = mode;
	/* set up addresses */
	address.sin_family = AF_INET;
	address.sin_port = INST_UDP_PORT;
	address.sin_addr.s_addr = INST_UDP_ADDR;
	myaddress.sin_family = AF_INET;
	myaddress.sin_port = 0;
	myaddress.sin_addr.s_addr = INADDR_ANY;
	/* send packet */
	_bind(s,&myaddress,sizeof(myaddress));
	sendto(s,&packet,sizeof(packet),0,&address,sizeof(address));
	close(s);
}

#endif	/* INSTRUMENT */
