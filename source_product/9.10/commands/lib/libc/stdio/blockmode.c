#ifdef _NAMESPACE_CLEAN
#define fstat _fstat
#define ioctl _ioctl
#define read _read
#define blclose _blclose
#define blget _blget
#define blopen _blopen
#define blread _blread
#define blset _blset
#endif

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/errno.h>
#include	<sys/blmodeio.h>
#include	<sys/param.h>  /* for the MAXFUPLIM value */

#ifndef MAXFUPLIM
#  define MAXFUPLIM 2048
#endif

#define		CBTRIG1		0x11	/* DC1 */
#define		CBTRIG2		0x11	/* DC1 */
#define		CBALERT		0x12	/* DC2 */
#define		CBTERM		0x1E	/* RS */
#define		BMUNDEF		0377	/* Undefined */

#define  	SETFD(fd)	(blop[(fd)/8] |= 1<<((fd) % 8))
#define  	CLRFD(fd)	(blop[(fd)/8] &= ~(1<<((fd) % 8)))
#define  	IS_FD_SET(fd)	(blop[(fd)/8] & (1<<((fd) % 8)))

static unsigned char blop[(MAXFUPLIM + 7) / 8];

extern int errno;

#ifdef _NAMESPACE_CLEAN
#undef blopen
#pragma _HP_SECONDARY_DEF _blopen blopen
#define blopen _blopen
#endif
blopen(fdes)
int fdes;
{
	struct blmodeio blm;		/* Used to initialize blmode chars */
	struct stat st;
	int ctr;

	if (fstat(fdes, &st) < 0) {	 /* Make sure it is a good fd */
		errno = ENOTTY;
		return -1;
	}

	if (IS_FD_SET(fdes)) {		/* Bad file descriptor, already open */
		errno = ENOTTY;
		return -1;
	}

	blm.cb_flags = 0;
	blm.cb_trig1c = CBTRIG1;	/* Trigger 1 - DC1 */
	blm.cb_trig2c = CBTRIG2;	/* Trigger 2 - DC1 */
	blm.cb_alertc = CBALERT;	/* Alert - DC2 */
	blm.cb_termc = CBTERM;		/* Terminator - RS */
	blm.cb_replen = 0;

	for (ctr = 0; ctr < NBREPLY; ctr++)
		blm.cb_reply[ctr] = 0;

	if (ioctl(fdes,CBSETA,&blm) < 0) {
		errno = ENOTTY;
		return -1;
	}

	SETFD(fdes);

	return fdes;
}

#ifdef _NAMESPACE_CLEAN
#undef blclose
#pragma _HP_SECONDARY_DEF _blclose blclose
#define blclose _blclose
#endif
blclose(bfd)
int bfd;
{
	struct blmodeio blm;		/* Used to initialize blmode chars */

	if (! IS_FD_SET(bfd)) {		/* Bad file descriptor */
		errno = ENOTTY;
		return -1;
	}

	blm.cb_flags = 0;
	blm.cb_trig1c = BMUNDEF;	/* Undefined */
	blm.cb_trig2c = BMUNDEF;	/* Undefined */
	blm.cb_alertc = BMUNDEF;	/* Undefined */
	blm.cb_termc =  BMUNDEF;	/* Undefined */
	blm.cb_replen = 0;

	if (ioctl(bfd,CBSETA,&blm) < 0) {
		errno = ENOTTY;
		return -1;
	}

	CLRFD(bfd);

	return 0;
}

#ifdef _NAMESPACE_CLEAN
#undef blget
#pragma _HP_SECONDARY_DEF _blget blget
#define blget _blget
#endif
blget(bfd,arg)
int bfd;
struct blmodeio *arg;
{
	if (! IS_FD_SET(bfd)) {		/* Bad file descriptor */
		errno = ENOTTY;
		return -1;
	}

	if (ioctl(bfd, CBGETA, arg) < 0) {
		errno = ENOTTY;
		return -1;
	}

	return 0;
}

#ifdef _NAMESPACE_CLEAN
#undef blset
#pragma _HP_SECONDARY_DEF _blset blset
#define blset _blset
#endif
blset(bfd,arg)
int bfd;
struct blmodeio *arg;
{
	if (! IS_FD_SET(bfd)) {		/* Bad file descriptor */
		errno = ENOTTY;
		return -1;
	}

	if (arg->cb_replen > NBREPLY)
	{
		errno = EINVAL;
		return -1;
	}

	if (ioctl(bfd, CBSETA, arg) < 0) {
		errno = ENOTTY;
		return -1;
	}

	return 0;
}

#ifdef _NAMESPACE_CLEAN
#undef blread
#pragma _HP_SECONDARY_DEF _blread blread
#define blread _blread
#endif
blread(bfd,adr,cnt)
int bfd, cnt;
char *adr;
{
	int ret;

	if (! IS_FD_SET(bfd)) {		/* Bad file descriptor */
		errno = ENOTTY;
		return -1;
	}

	if (cnt <= 0)
		return 0;

	if ((ret = read(bfd,adr,cnt)) < 0)
		return -1;

	return ret;
}
