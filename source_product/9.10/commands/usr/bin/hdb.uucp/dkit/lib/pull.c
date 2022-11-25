/*	@(#) $Revision: 64.1 $	*/
/*
 *	COMMKIT(TM) Software - Datakit(R) VCS Interface Release 2.0 V1
 *			Copyright 1984 AT&T
 *			All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *     The copyright notice above does not evidence any actual
 *          or intended publication of such source code.
 */

#include	<errno.h>
#include	<fcntl.h>
#include	<string.h>
#include	<varargs.h>
#include	"pupu.h"
#ifdef	SVR3
#include	<dirent.h>
#else
#include	<ndir.h>
#endif

	Efn	takefile, fetchrequest, setdirectory, terminate;
	Efn	takelink, confirmed, remoteerror, alldone, remotefail;

	Sexp	pulltable[] = {
		'F',	takefile,
		'L',	takelink,
		'S',	fetchrequest,
		'D',	setdirectory,
		'X',	terminate,
		0,	0
	};

	Sexp	fetchtable[] = {
		'F',	takefile,
		'L',	takelink,
		'c',	confirmed,
		'e',	remoteerror,
		'x',	remotefail,
		'X',	alldone,
		0,	0
	};

	Eexp	justquit[];

	Sint	dochown;

	EcharP	below();
	EpwP	getpwnam();

pull(fd, names, directory, errfcn, options)
	PfnP	errfcn;
	PcharP	*names, directory, options;
{
	begin(fd, errfcn, "local", ".");

	dochown = (geteuid() == 0);

	if(setjmp(giveup))
		return(lasterror);

	sendinit(options);

	setbase(directory);

	while(*names)
		fetchfile(*names++);

	msginit('X');
	msgsend();
	msgwait(justquit);

	signal(SIGALRM, alarmwas);
	return(lasterror);
}

fetchfile(name)
	PcharP	name;
{
	msginit('S');

	msgfmt("%s", name);

	msgconf(fetchtable);
}

fetchrequest()
{
	Achar	name[NAMELEN];

	msgtake("%s", name);

	sendfile(name);

	reply('c');

	return(0);
}

ppremote(fd, home)
	PcharP	home;
{
	begin(fd, (TfnP) 0, "remote", home ? home : "");

	dochown = (geteuid() == 0);

	if(setjmp(giveup))
		return(lasterror);

	msgwait(inittable);

	msgwait(pulltable);

	signal(SIGALRM, alarmwas);
	return(lasterror);
}

setdirectory()
{
	setbase(&buf[H_DATA]);

	reply('c');

	return(0);
}

setbase(p)
	RcharP	p;
{
	RcharP	q;
	RpwP	pw;

	if(*p == '/')
		strcpy(basedir, p);
	else if(*p == '~'){
		if(q = strchr(++p, '/'))
			*q++ = '\0';

		if(*p){
			if(pw = getpwnam(p))
				strcpy(basedir, pw->pw_dir);
			else
				err("No passwd entry for", p, -EX_NOUSER);
		}else{
#if hpux
			/*
			 * get the name of the current directory.
			 */
			getcwd(basedir, NAMELEN);
#else
			curdir(basedir);
#endif
			strcpy(wdir, basedir);
		}

		if(q){
			strcat(basedir, "/");
			strcat(basedir, q);
		}
	}else{
#if hpux
		/*
		 * get the name of the current directory.
		 */
		getcwd(basedir, NAMELEN);
#else
		curdir(basedir);
#endif
		strcpy(wdir, basedir);

		if(strcmp(p, ".") != 0){
			strcat(basedir, "/");
			strcat(basedir, p);
		}
	}

db(stderr, "setdirectory %s\n", basedir);
	changedir(basedir);

	return(0);
}

changedir(newdir)
	PcharP	newdir;
{
	RcharP	p;

	if(!(p = below(wdir, newdir)))
		p = newdir;

	if(*p){
		if(chdir(p) < 0 && (makedirectory(p), chdir(p) < 0))
			err("Can't chdir to", p, EX_NOINPUT);

		strcpy(wdir, newdir);
	}
db(stderr, "changedir newdir %s p %s wdir %s\n", newdir, p, wdir);
}

	PcharP
below(ref, check)
	RcharP	ref, check;
{
	Rint	len;

	len = strlen(ref);

	if(strncmp(ref, check, len) != 0)
		return(NIL);

	check += len;

	if(*check == '/')
		return(check+1);
	if(*check)
		return(NIL);
	return(check);
}

takefile()
{
	RcharP	p;
	Rint	haderr;
	Achar	type;
	Achar	name[NAMELEN];
	Aint	mode, uid, gid;
	Along	mtime;
	time_t	timea[2];

	msgtake("%c", &type);
	msgtake("%s", name);
	msgtake("%o", &mode);
	msgtake("%d", &uid);
	msgtake("%d", &gid);
	msgtake("%ld", &mtime);

	sprintf(wholename, "%s/%s", basedir, name);

	p = strrchr(wholename, '/');
	*p = '\0';

	changedir(wholename);

	*p++ = '/';

	switch(type){
	case 'r':
		haderr = pluckregular(p);
		break;

	case 'd':
		makedirectory(p);
		haderr = 0;
		break;

	case 'c':
		haderr = pluckspecial(p, S_IFCHR);
		break;

	case 'b':
		haderr = pluckspecial(p, S_IFBLK);
		break;

	case 'p':
		haderr = pluckspecial(p, S_IFIFO);
	}

	reply('c');

	if(haderr)
		return(0);

	chmod(p, mode);

	timea[0] = time((TlongP) 0);
	timea[1] = mtime;

	if(timea[1] >= timea[0])
		timea[1] = timea[0] - 1;

	utime(p, timea);

	if(dochown)
		chown(p, uid, gid);

	return(0);
}

pluckregular(p)
	PcharP	p;
{
	Rint	ffd, len;
	Rint	status	= GOOD;
	Along	sentsize, receivedsize;

	if((ffd = openn(p, O_WRONLY|O_CREAT|O_TRUNC, 0200)) < 0
	  && (chmod(p, 0200), (ffd = openn(p, O_WRONLY|O_CREAT|O_TRUNC, 0200)) < 0)){

		warn("Can't create", wholename, EX_CANTCREAT);

		while(read(cfd, buf, sizeof(buf)) > 0);

		return(BAD);
	}

	msgtake("%ld", &sentsize);
	receivedsize = 0;

	currentfile = wholename;
	timeoutmsg = "File reception timed out";

	alarm(RTIMEOUT);

	while((len = read(cfd, buf, sizeof(buf))) > 0){
db(stderr, "len %d\n", len);
		if(write(ffd, buf, len) != len){
			warn("Write failed on", wholename, EX_IOERR);
			while((len = read(cfd, buf, sizeof(buf))) > 0);

			status = BAD;

			break;
		}
		receivedsize += len;
		alarm(RTIMEOUT);
	}

	alarm(0);

	if(len < 0)
		stopnow(EX_IOERR);
	if(sentsize != receivedsize){
		if(sentsize > receivedsize)
#if hpux
		{
			char wbuf[60];

			sprintf(wbuf, "File shrank(%d bytes) during transmission", sentsize - receivedsize);
			warn(wbuf, wholename, -EX_IOERR);
		}
#else
			warn("File shrank during transmission", wholename, -EX_IOERR);
#endif
		else
			warn("File grew during transmission", wholename, -EX_IOERR);
		status = BAD;
	}

	closen(ffd);

	return(status);
}

pluckspecial(name, mode)
	PcharP	name;
{
	Rint	dev;
	Aint	maj, min;

	unlink(name);

	if(mode == S_IFIFO)
		dev = 0;
	else{
		msgtake("%d", &maj);
		msgtake("%d", &min);

		dev = makedev(maj, min);
	}

	if(mknod(name, mode, dev) < 0){
		warn("Can't mknod", wholename, EX_CANTCREAT);
		return(BAD);
	}

	return(GOOD);
}

takelink()
{
	RcharP	p;
	Achar	name[NAMELEN], prev[NAMELEN];

	msgtake("%s", name);
	msgtake("%s", prev);

	sprintf(wholename, "%s/%s", basedir, name);

	p = strrchr(wholename, '/');
	*p = '\0';
	changedir(wholename);
	*p++ = '/';

	sprintf(buf, "%s/%s", basedir, prev);

	if((link(buf, p) < 0) && (unlink(p), (link(buf, p) < 0)))
		warn("Can't link to", wholename, EX_CANTCREAT);

	reply('c');

	return(0);
}

makedirectory(p)
	PcharP	p;
{
	RcharP	q;
	Aint	pid, n, result;
	Achar	temp[NAMELEN];

db(stderr, "makedirectory %s\n", p);
	sprintf(temp, "%s/", p);

	q = temp;
	if(*q == '/')
		q++;

	while(q = strchr(q, '/')){
		*q = '\0';

		if(access(temp, 1) < 0){
db(stderr, "mkdir %s\n", temp);
#ifdef	SVR3
			if (mkdir(temp, 0777))
				warn("Can't make directory", temp, -EX_CANTCREAT);
#else
			if((pid = fork()) == 0){
				freopen("/dev/null", "w", stderr);
				execl("/bin/mkdir", "mkdir", temp, 0);
				err("No mkdir", NIL, EX_UNAVAILABLE);
				/* NOTREACHED */
			}
			while((n = wait(&result)) > 0 && n != pid);

			if(result != 0)
				warn("Can't make directory", temp, -EX_CANTCREAT);
#endif
		}

		*q++ = '/';
	}
}

terminate()
{
	msginit('x');
	msgfmt("%d", lasterror);
	msgsend();

	return(1);
}
