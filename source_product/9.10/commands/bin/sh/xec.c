/* @(#) $Revision: 70.4 $ */
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */


#include	"defs.h"
#include	<errno.h>
#include	"sym.h"
#include	"hash.h"
#if defined(SYSNETUNAM) && defined(RFA)
#include	<errnet.h>
#include	<pwd.h>
#endif /* SYSNETUNAM && RFA */

#ifdef NLS
#define NL_SETN 1
#endif

static int	parent;

#if defined(SYSNETUNAM) && defined(RFA)
int in_netunam;
#endif /* SYSNETUNAM && RFA */

int (*oldsigwinch)();

/* ========	command execution	========*/


execute(argt, exec_link, errorflg, pf1, pf2)
struct trenod	*argt;
int	*pf1, *pf2;
{
	/*
	 * `stakbot' is preserved by this routine
	 */
	register struct trenod	*t;
	tchar		*sav = savstak();
	int (*oldsigcld)();

	sigchk();
	if (!errorflg)
		flags &= ~errflg;

	if ((t = argt) && execbrk == 0)
	{
		register int	treeflgs;
		int 			type;
		register tchar	**com;
		short			pos;
		int 			linked;
		int 			execflg;

		linked = exec_link >> 1;
		execflg = exec_link & 01;

		treeflgs = t->tretyp;
		type = treeflgs & COMMSK;

		switch (type)
		{
		case TFND:
		{
			struct fndnod	*f = (struct fndnod *)t;
			struct tnamnod	*n = lookup(f->fndnam);

			exitval = 0;

			if (n->namflg & N_RDONLY)
				tfailed(n->namid, nl_msg(627,wtfailed));

			if (n->namflg & N_FUNCTN)
				freefunc(n);
			else
			{
				free(n->namval);
				free(n->namenv);

				n->namval = 0;
				n->namflg &= ~(N_EXPORT | N_ENVCHG);
			}

			if (funcnt)
				f->fndval->tretyp++;

			n->namenv = (tchar *)f->fndval;
			attrib(n, N_FUNCTN);
			hash_func(n->namid);
			break;
		}

		case TCOM:
		{
			tchar	*a1;
			int	argn, internal;
			struct argnod	*schain = gchain;
			struct ionod	*io = t->treio;
			short 	cmdhash;
			short	comtype;

			exitval = 0;

			gchain = 0;
			argn = getarg(t);
			com = scan(argn);
			a1 = com[1];
			gchain = schain;

			if (argn != 0)
				cmdhash = pathlook(com[0], 1, comptr(t)->comset);

			if (argn == 0 || (comtype = hashtype(cmdhash)) == BUILTIN)
				setlist(comptr(t)->comset, 0);

			if (argn && (flags&noexec) == 0)
			{
				/* print command if execpr */

				if (flags & execpr)
					execprint(com);

				if (comtype == NOTFOUND)
				{
					char *errstr;

					pos = hashdata(cmdhash);
					if (pos == 1)
						errstr = (char *)nl_msg(622,notfound);
					else if (pos == 2)
						errstr = (char *)nl_msg(621,badexec);
					else
						errstr = (char *)nl_msg(634,badperm);
					prp();
					prst_cntl(*com);
					prs(colon);
					prs(errstr);
					newline();
					exitval = 1;
					break;
				}

				else if (comtype == PATH_COMMAND)
				{
					pos = -1;
				}

				else if (comtype & (COMMAND | REL_COMMAND))
				{
					pos = hashdata(cmdhash);
				}

				else if (comtype == BUILTIN)
				{
					short index;

					internal = hashdata(cmdhash);
					index = initio(io, (internal != SYSEXEC));

					switch (internal)
					{
					case SYSDOT:
						if (a1)
						{
							register int	f;

							if ((f = pathopen(getpath(a1), a1)) < 0)
								tfailed(a1, nl_msg(622,notfound));
							else {
                                                                if(argn > 2)
                                                                 setargs(com+1);
                                                                execexp(0, f);
                                                             }
                                                                
						}
						break;

					case SYSTIMES:
						{
							long int t[4];

							times(t);
							prt(t[2]);
							prc_buff(SP);
							prt(t[3]);
							prc_buff(NL);
						}
						break;

					case SYSEXIT:
						flags |= forked;	/* force exit */
						exitsh(a1 ? stoi(a1) : retval);

					case SYSNULL:
						io = 0;
						break;

					case SYSCONT:
						if (loopcnt)
						{
							execbrk = breakcnt = 1;
							if (a1)
								breakcnt = stoi(a1);
							if (breakcnt > loopcnt)
								breakcnt = loopcnt;
							else
								breakcnt = -breakcnt;
						}
						break;

					case SYSBREAK:
						if (loopcnt)
						{
							execbrk = breakcnt = 1;
							if (a1)
								breakcnt = stoi(a1);
							if (breakcnt > loopcnt)
								breakcnt = loopcnt;
						}
						break;

					case SYSTRAP:
						if (a1)
						{
							BOOL 	clear;

							if ((clear = digit(*a1)) == 0)
								++com;
							while (*++com)
							{
								int	i;

								if ((i = stoi(*com)) >= MAXTRAP || i < MINTRAP)
									tfailed(*com, nl_msg(626,badtrap));
								else if (clear)
									clrsig(i);
								else
								{
									replace(&trapcom[i], a1);
									if (*a1)
										getsig(i);
									else
										ignsig(i);
								}
							}
						}
						else	/* print out current traps */
						{
							int	i;

							for (i = 0; i < MAXTRAP; i++)
							{
								if (trapcom[i])
								{
									prn_buff(i);
									prs_buff(colon);
									prst_buff(trapcom[i]);
									prc_buff(NL);
								}
							}
						}
						break;

					case SYSEXEC:
						com++;
						ioset = 0;
						io = 0;
						if (a1 == 0)
						{
							break;
						}

#ifdef RES	/* Research includes login as part of the shell */

					case SYSLOGIN:
						oldsigs();
						execa(com, -1);
						done();
#else

					case SYSNEWGRP:
						if (flags & rshflg)
							tfailed(com[0], nl_msg(614,restricted));
						else
						{
							flags |= forked;	/* force bad exec to terminate shell */
							oldsigs();
							execa(com, -1);
							done();
						}

#endif

					case SYSCD:
						if (flags & rshflg)
							tfailed(com[0], nl_msg(614,restricted));
						else if ((a1 && *a1) || (a1 == 0 && (a1 = homenod.namval)))
						{
							tchar *cdpath;
							tchar *dir;
							int f;

							if ((cdpath = cdpnod.namval) == 0 ||
							     *a1 == '/' ||
							     cf(a1, to_tchar(".")) == 0 ||
#if defined(DUX) || defined(DISKLESS)
							     cf(a1, to_tchar(".+")) == 0 ||
							     cf(a1, to_tchar("..+")) == 0 ||
#endif
							     cf(a1, to_tchar("..")) == 0 ||
#if defined(DUX) || defined(DISKLESS)
							     (*a1 == '.' && *(a1+1) == '+'  && *(a1+2) == '/'	) ||
                                                             (*a1 == '.' && *(a1+1) == '.'  && *(a1+2) == '+' &&
                                                                                                       *(a1+3) == '/') ||
#endif
							     (*a1 == '.' && (*(a1+1) == '/' || *(a1+1) == '.' && *(a1+2) == '/')))
								cdpath = (tchar *)nullstr;

							do
							{
								dir = cdpath;
								cdpath = catpath(cdpath,a1);
							}
							while ((f = (chdir(to_char(curstak())) < 0)) && cdpath);

							if (f)
								tfailed(a1, nl_msg(625,baddir));
							else
							{
								cwd(curstak());
								if (cf(nullstr, dir) &&
								    *dir != ':' &&
								 	any('/', curstak()) &&
								 	flags & prompt)
								{
									prst_buff(curstak());
									prc_buff(NL);
								}
							}
							zapcd();
						}
						else
						{
							if (a1)
								tfailed(a1, nl_msg(625,baddir));
							else
								error(nl_msg(633,nohome));
						}

						break;

					case SYSSHFT:
						{
							int places;

							places = a1 ? stoi(a1) : 1;

							if ((dolc -= places) < 0)
							{
								dolc = 0;
								error(nl_msg(624,badshift));
							}
							else
								dolv += places;
						}

						break;

					case SYSWAIT:
						oldsigcld = (int(*)())signal(SIGCLD, SIG_DFL);
						await(a1 ? stoi(a1) : -1, 1);
						signal(SIGCLD, oldsigcld);
						break;

					case SYSREAD:
						rwait = 1;
						exitval = readvar(&com[1]);
						rwait = 0;
						break;

					case SYSSET:
						if (a1)
						{
							int	argc;

							argc = options(argn, com);
							if (argc > 1)
								setargs(com + argn - argc);
						}
						else if (comptr(t)->comset == 0)
						{
							/*
							 * scan name chain and print
							 */
							namscan(printnam);
						}
						break;

					case SYSRDONLY:
						exitval = 0;
						if (a1)
						{
							while (*++com)
								attrib(lookup(*com), N_RDONLY);
						}
						else
							namscan(printro);

						break;

					case SYSXPORT:
						{
							struct tnamnod 	*n;

							exitval = 0;
							if (a1)
							{
								while (*++com)
								{
									n = lookup(*com);
									if (n->namflg & N_FUNCTN)
										error(nl_msg(631,badexport));
									else
										attrib(n, N_EXPORT);
								}
							}
							else
								namscan(printexp);
						}
						break;

					case SYSEVAL:
						if (a1)
							execexp(a1, &com[2]);
						break;

#ifndef RES
					case SYSULIMIT:
						{
							long int i;
							long ulimit();
							int command = 2;

							if ((a1) && (*a1 == '-'))
							{	switch(*(a1+1))
								{
									case 'f':
										command = 2;
										break;

#ifdef rt
									case 'p':
										command = 5;
										break;

#endif

									default:
										error(nl_msg(601,badopt));
								}
								a1 = com[2];
							}
							if (a1)
							{
								int c;

								i = 0;
								while ((c = *a1++) >= '0' && c <= '9')
								{
									i = (i * 10) + (long)(c - '0');
									if (i < 0)
										error(nl_msg(629,badulimit));
								}
								if (c || i < 0)
									error(nl_msg(629,badulimit));
							}
							else
							{
								i = -1;
								command--;
							}

							if ((i = ulimit(command,i)) < 0)
								error(nl_msg(629,badulimit));

							if (command == 1 || command == 4)
							{
								prl(i);
								prc_buff('\n');
							}
							break;
						}

					case SYSUMASK:
						if (a1)
						{
							int c, i;

							i = 0;
							while ((c = *a1++) >= '0' && c <= '7')
								i = (i << 3) + c - '0';
							umask(i);
						}
						else
						{
							int i, j;

							umask(i = umask(0));
							prc_buff('0');
							for (j = 6; j >= 0; j -= 3)
								prc_buff(((i >> j) & 07) +'0');
							prc_buff(NL);
						}
						break;

#endif

					case SYSTST:
						exitval = test(argn, com);
						break;

					case SYSECHO:
						exitval = echo(argn, com);
						break;

					case SYSHASH:
						exitval = 0;

						if (a1)
						{
							if (a1[0] == '-')
							{
								if (a1[1] == 'r')
									zaphash();
								else
									error(nl_msg(601,badopt));
							}
							else
							{
								while (*++com)
								{
									if (hashtype(hash_cmd(*com)) == NOTFOUND)
										tfailed(*com, nl_msg(622,notfound));
								}
							}
						}
						else
							hashpr();

						break;

					case SYSPWD:
						{
							exitval = 0;
#if defined(DUX) || defined(DISKLESS)
                                               if (cf(a1,to_tchar("-H")) == 0)
                                                        cwdprint(1);
                                               else cwdprint(0);
#else
							cwdprint();
#endif
						}
						break;

					case SYSRETURN:
						if (funcnt == 0)
							error(nl_msg(630,badreturn));

						execbrk = 1;
						exitval = (a1 ? stoi(a1) : retval);
						break;

					case SYSTYPE:
						exitval = 0;
						if (a1)
						{
							while (*++com)
								what_is_path(*com);
						}
						break;

					case SYSUNS:
						exitval = 0;
						if (a1)
						{
							while (*++com)
								unset_name(*com);
						}
						break;

#if defined(SYSNETUNAM) && defined(RFA)
	case SYSNETUNAM:
		{	extern	errnet;
			char	*s;
			char	*cuserid();
			char	*login_str;
			char	*lanlogin();
			struct	passwd	*p;
			struct	passwd	*getpwnam();
/*
 * To make the error messages work properly on netunam, I will make
 * all messages go through failed.  Since I want to print out
 * integers I need to convert them into strings and then concatenate
 * a message with the number string to form the string to be
 * given to  failed.  Therefore it is important to not build a
 * message greater than  MESS_LEN in length or to increase MESS_LEN.
 * The string pointed to by badexec is 14 characters long and numbuf
 * could be up to 12 characters long, but really should not be
 * longer than 4 digits.
 * The routine itos uses a global buffer called numbuf to do its
 * conversion which is why it is declared here.   Mike Shipley CNO 01/18/85
 */

#define MESS_LEN  120
#ifndef NLS
			extern char numbuf[12];  /*mikey*/
#else NLS
			extern char cnumbuf[12];
			char	l_com[128];
#define 	numbuf 	cnumbuf
#define 	itos 	itosc
#endif NLS
			char err_mess[MESS_LEN]; /*mikey*/
			int (*oldsigsys)(), ret; /* add signal handler for sigsys - mn */

			if (((s = cuserid(0)) == (char *) NIL) || ((p = getpwnam(s)) == (struct passwd *) NIL))
				tfailed(com[0], nl_msg(621,badexec));
			else {
				endpwent();
				if (argn != 3)
  					failed(to_char(com[0]), (nl_msg(1001, "Usage: netunam  network_special_file  login_string")));
				else {
               				login_str = lanlogin(sto_char(com[2],l_com));
						oldsigsys = signal(SIGSYS, fault);
						in_netunam = 0;
       					ret = netunam(to_char(com[1]), login_str);
						signal(SIGSYS, oldsigsys);
						if (in_netunam == 1){  /* sigsys was trapped */
							ret = 1;
							failed(com[0], nl_msg(1007, "unsupported network call"));
						}
       				if (ret < 0) {
					*err_mess = '\0';  /*mikey*/
					if (errno == ENET) {
 						if (errnet == NE_NOLOGIN) {
							strncat(err_mess,
							  (nl_msg(1002, "invalid remote login.  errnet = ")), MESS_LEN);
							itos(errnet);       /*mikey*/
							strncat(err_mess, numbuf,
							   MESS_LEN);  /*mikey*/

						}
  						else if (errnet == NE_CONNLOST) {
							strncat(err_mess,
							  (nl_msg(1003, "remote node not answering.  errnet = ")), MESS_LEN);
							itos(errnet);       /*mikey*/
							strncat(err_mess, numbuf,
							   MESS_LEN);  /*mikey*/
						}
						else {
							strncat(err_mess, nl_msg(621,badexec), MESS_LEN);  /*mikey*/
							strncat(err_mess,
							   ".  errnet = ",
							   MESS_LEN);  /*mikey*/
							itos(errnet);       /*mikey*/
							strncat(err_mess, numbuf,
							MESS_LEN);  /*mikey*/
						}
					}
					else if (errno == ENOENT) {
						strncat(err_mess,
						  (nl_msg(1004, "invalid network special file.  errno = ")), MESS_LEN);
						itos(ENOENT);        /*mikey*/
						strncat(err_mess, numbuf,
							MESS_LEN);  /*mikey*/
					}
					else {
						int tmperrno = errno; /* save network's errno value */
						strncat(err_mess, nl_msg(621,badexec), MESS_LEN);  /*mikey*/
						strncat(err_mess, ".  errno = ",
							MESS_LEN);  /*mikey*/
						itos(tmperrno);        /*mikey*/
						strncat(err_mess, numbuf,
							MESS_LEN);  /*mikey*/
					}
					failed(to_char(com[0]), err_mess); /*mikey*/
					}
				}
			}
#ifdef NLS
#undef itos
#undef numbuf
#endif NLS
		}
		break;
#endif /* SYSNETUNAM && RFA */
					default:
						prs_buff((nl_msg(1005, "unknown builtin\n")));
					}


					flushb();
					restore(index);
					chktrap();
					break;
				}

				else if (comtype == FUNCTION)
				{
					struct tnamnod *n;
					short index;

					n = findnam(com[0]);

					funcnt++;
					index = initio(io, 1);
					setargs(com);
					execute((struct trenod *)(n->namenv), exec_link, errorflg, pf1, pf2);
					execbrk = 0;
					restore(index);
					funcnt--;

					break;
				}
			}
			else if (t->treio == 0)
			{
				chktrap();
				break;
			}

		}

		case TFORK:
			exitval = 0;
			if (execflg && (treeflgs & (FAMP | FPOU)) == 0)
				parent = 0;
			else
			{
				int forkcnt = 1;

				if (treeflgs & (FAMP | FPOU))
				{
					link_iodocs(iotemp);
					linked = 1;
				} else {
					/*
					 * If this is a forground process
					 * set its action to SIG_DFL.  We'll
					 * set it back to fault (oldsigcld)
					 * after the return from await.  This
					 * catches the case when the SIGCLD
					 * arived before we did a wait().
					 * part of bell bug fix.
					 */
					oldsigcld = (int(*)())signal(SIGCLD, SIG_DFL);
				}


				/*
				 * FORKLIM is the max period between forks -
				 * power of 2 usually.  Currently shell tries
				 * after 2, 4 and 8 seconds and then quits
				 */

				while ((parent = fork()) == -1)
				{
					if ((forkcnt = (forkcnt * 2)) > FORKLIM)
					{
						int tmperrno = errno; /* save network's error value */
						switch (errno)
						{
						case ENOMEM:
						case ENOSPC:
							error(nl_msg(613,noswap));
							break;
						case EAGAIN:
							error(nl_msg(612,nofork));
							break;
						default:
							prs((nl_msg(1006, "cannot fork: errno = ")));
							prn(tmperrno);
							error("");
							break;
						}
					}
					sigchk();
					alarm(forkcnt);
					pause();
					/*
					 * Need alarm(0) as pause might have
					 * terminated from a SIGCLD.
					 */
					alarm(0);
				}
			}
			if (parent)
			{
                                /* ignore SIGWINCH for now and store off
				 * handler for later 
                                 */
				oldsigwinch = (int(*)())signal(SIGWINCH,SIG_IGN);

				/*
				 * This is the parent branch of fork;
				 * it may or may not wait for the child
				 */
				if (treeflgs & FPRS && flags & ttyflg)
				{
					prn(parent);
					newline();
				}
				if (treeflgs & FPCL)
					closepipe(pf1);
				if ((treeflgs & (FAMP | FPOU)) == 0) {
					await(parent, 0);
					signal(SIGCLD, oldsigcld);
				} else if ((treeflgs & FAMP) == 0)
					post(parent);
				else
					assnum(&pcsadr, parent);
				chktrap();
				break;
			}
			else	/* this is the forked branch (child) of execute */
			{
				flags |= forked;
				fiotemp  = 0;

				if (linked == 1)
				{
					swap_iodoc_nm(iotemp);
					exec_link |= 06;
				}
				else if (linked == 0)
					iotemp = 0;

#ifdef ACCT
				suspacct();
#endif

				postclr();
				settmp();
				/*
				 * Turn off INTR and QUIT if `FINT'
				 * Reset ramaining signals to parent
				 * except for those `lost' by trap
				 */
				oldsigs();
				if (treeflgs & FINT)
				{
					signal(SIGINT, 1);
					signal(SIGQUIT, 1);

#ifdef NICE
					nice(NICEVAL);
#endif

				}
				/*
				 * pipe in or out
				 */
				if (treeflgs & FPIN)
				{
					rename(pf1[INPIPE], 0);
					close(pf1[OTPIPE]);
				}
				if (treeflgs & FPOU)
				{
					close(pf2[INPIPE]);
					rename(pf2[OTPIPE], 1);
				}
				/*
				 * default std input for &
				 */
				if (treeflgs & FINT && ioset == 0)
					rename(chkopen(devnull), 0);
				/*
				 * io redirection
				 */
				initio(t->treio, 0);

				if (type != TCOM)
				{
					execute(forkptr(t)->forktre, exec_link | 01, errorflg);
				}
				else if (com[0] != ENDARGS)
				{
					eflag = 0;
					setlist(comptr(t)->comset, N_EXPORT);
					rmtemp(0);
					execa(com, pos);
				}
				done();
			}

		case TPAR:
			execute(parptr(t)->partre, exec_link, errorflg);
			done();

		case TFIL:
			{
				int pv[2];

				chkpipe(pv);
				if (execute(lstptr(t)->lstlef, 0, errorflg, pf1, pv) == 0)
					execute(lstptr(t)->lstrit, exec_link, errorflg, pv, pf2);
				else
					closepipe(pv);
			}
			break;

		case TLST:
			execute(lstptr(t)->lstlef, 0, eflag);
			execute(lstptr(t)->lstrit, exec_link, eflag);
			break;

		case TAND:
			if (execute(lstptr(t)->lstlef, 0, 0) == 0)
				execute(lstptr(t)->lstrit, exec_link, errorflg);
			break;

		case TORF:
			if (execute(lstptr(t)->lstlef, 0, 0) != 0)
				execute(lstptr(t)->lstrit, exec_link, errorflg);
			break;

		case TFOR:
			{
				struct tnamnod *n = lookup(forptr(t)->fornam);
				tchar	**args;
				struct dolnod *argsav = 0;

				if (forptr(t)->forlst == 0)
				{
					args = dolv + 1;
					argsav = useargs();
				}
				else
				{
					struct argnod *schain = gchain;

					gchain = 0;
					trim((args = scan(getarg(forptr(t)->forlst)))[0]);
					gchain = schain;
				}
				loopcnt++;
				while (*args != ENDARGS && execbrk == 0)
				{
					assign(n, *args++);
					execute(forptr(t)->fortre, 0, errorflg);
					if (breakcnt < 0)
						execbrk = (++breakcnt != 0);
				}
				if (breakcnt > 0)
						execbrk = (--breakcnt != 0);

				loopcnt--;
				argfor = (struct dolnod *)freeargs(argsav);
			}
			break;

		case TWH:
		case TUN:
			{
				int	i = 0;

				loopcnt++;
				while (execbrk == 0 && (execute(whptr(t)->whtre, 0, 0) == 0) == (type == TWH) && (flags & noexec) == 0)
				{
					i = execute(whptr(t)->dotre, 0, errorflg);
					if (breakcnt < 0)
						execbrk = (++breakcnt != 0);
				}
				if (breakcnt > 0)
						execbrk = (--breakcnt != 0);

				loopcnt--;
				exitval = i;
			}
			break;

		case TIF:
			if (execute(ifptr(t)->iftre, 0, 0) == 0)
				execute(ifptr(t)->thtre, exec_link, errorflg);
			else if (ifptr(t)->eltre)
				execute(ifptr(t)->eltre, exec_link, errorflg);
			else
				exitval = 0;	/* force zero exit for if-then-fi */
			break;

		case TSW:
			{
				register tchar	*r = mactrim(swptr(t)->swarg);
				register struct regnod *regp;

				regp = swptr(t)->swlst;
				while (regp)
				{
					struct argnod *rex = regp->regptr;

					while (rex)
					{
						register tchar	*s;

						if (gmatch(r, s = macro(rex->argval)) || (trim(s), eqtt(r, s)))
						{
							execute(regp->regcom, 0, errorflg);
							regp = 0;
							break;
						}
						else
							rex = rex->argnxt;
					}
					if (regp)
						regp = regp->regnxt;
				}
			}
			break;
		}
		exitset();
	}
	flags &= ~(noprompt | sigwtrap);
	sigchk();
	tdystak(sav);
	flags |= eflag;
	return(exitval);
}

execexp(s, f)
tchar	*s;
int	f;
{
	struct fileblk	fb;

	push(&fb);
	if (s)
	{
		estabf(s);
		fb.feval = (tchar **)(f);
	}
	else if (f >= 0)
		initf(f);
	execute(cmd(NL, NLFLG | MTFLG), 0, (int)(flags & errflg));
	pop();
}

execprint(com)
tchar **com;
{
	register int 	argn = 0;

	prs(execpmsg);

	while(com[argn] != ENDARGS)
	{
		prst(com[argn++]);
		blank();
	}

	newline();
}
