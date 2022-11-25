/* @(#) $Revision: 62.1 $ */      
/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */


#include	"defs.h"
#include	"hash.h"
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<errno.h>

#define		EXECUTE		01

#ifdef NLS
#define NL_SETN 1
#endif


static char	cost;
static int	dotpath;
static int	multrel;
static struct entry	*relcmd = 0;

int		argpath();


short
pathlook(com, flg, arg)
	tchar	*com;			
	int		flg;
	register struct argnod	*arg;
{
	register tchar	*name = com;	
	register ENTRY	*h;

	ENTRY		hentry;
	int		count = 0;
	int		i;
	int		pathset = 0;
	int		oldpath = 0;
#ifndef NLS
	struct namnod	*n;
#endif NLS


	hentry.data = 0;

	if (any('/', name))
		return(COMMAND);

	h = hfind(name);

	if (h)
	{
		if (h->data & (BUILTIN | FUNCTION))
		{
			if (flg)
				h->hits++;
			return(h->data);
		}

		if (arg && (pathset = argpath(arg)))
			return(PATH_COMMAND);

		if ((h->data & DOT_COMMAND) == DOT_COMMAND)
		{
			if (multrel == 0 && hashdata(h->data) > dotpath)
				oldpath = hashdata(h->data);
			else
				oldpath = dotpath;

			h->data = 0;
			goto pathsrch;
		}

		if (h->data & (COMMAND | REL_COMMAND))
		{
			if (flg)
				h->hits++;
			return(h->data);
		}

		h->data = 0;
		h->cost = 0;
	}

	if (i = syslook(name, commands, no_commands))
	{
		hentry.data = (BUILTIN | i);
		count = 1;
	}
	else
	{
		if (arg && (pathset = argpath(arg)))
			return(PATH_COMMAND);
pathsrch:
			count = findpath(name, oldpath);
	}

	if (count > 0)
	{
		if (h == 0)
		{
			hentry.cost = 0;
			hentry.key = make(name);
			h = henter(hentry);
		}

		if (h->data == 0)
		{
			if (count < dotpath)
				h->data = COMMAND | count;
			else
			{
				h->data = REL_COMMAND | count;
				h->next = relcmd;
				relcmd = h;
			}
		}


		h->hits = flg;
		h->cost += cost;
		return(h->data);
	}
	else 
	{
		return(-count);
	}
}
			

static void
zapentry(h)
	ENTRY *h;
{
	h->data &= HASHZAP;
}

void
zaphash()
{
	hscan(zapentry);
	relcmd = 0;
}

void 
zapcd()
{
	while (relcmd)
	{
		relcmd->data |= CDMARK;
		relcmd = relcmd->next;
	}
}


static void
hashout(h)
	ENTRY *h;
{
	sigchk();

	if (hashtype(h->data) == NOTFOUND)
		return;

	if (h->data & (BUILTIN | FUNCTION))
		return;

	prn_buff(h->hits);

	if (h->data & REL_COMMAND)
		prc_buff('*');


	prc_buff(TAB);
	prn_buff(h->cost);
	prc_buff(TAB);

	pr_path(h->key, hashdata(h->data));
	prc_buff(NL);
}

void
hashpr()
{
	prs_buff((nl_msg(401, "hits	cost	command\n")));
	hscan(hashout);
}


set_dotpath()
{
	register tchar	*path;		
	register int	cnt = 1;
	tchar zero = 0;	

	dotpath = 10000;
#ifndef NLS
	path = getpath("");
#else NLS
	path = getpath(&zero);
#endif NLS

	while (path && *path)
	{
		if (*path == '/')
			cnt++;
		else
		{
			if (dotpath == 10000)
				dotpath = cnt;
			else
			{
				multrel = 1;
				return;
			}
		}
	
		path = nextpath(path);
	}

	multrel = 0;
}


hash_func(name)
	tchar *name;
{
	ENTRY	*h;
	ENTRY	hentry;

	h = hfind(name);

	if (h)
	{

		if (h->data & (BUILTIN | FUNCTION))
			return;
		else
			h->data = FUNCTION;
	}
	else
	{
		int i;

		if (i = syslook(name, commands, no_commands))
			hentry.data = (BUILTIN | i);
		else
			hentry.data = FUNCTION;

		hentry.key = make(name);
		hentry.cost = 0;
		hentry.hits = 0;
	
		henter(hentry);
	}
}

func_unhash(name)
	tchar *name;
{
	ENTRY 	*h;

	h = hfind(name);

	if (h && (h->data & FUNCTION))
		h->data = NOTFOUND;
}


short
hash_cmd(name)
	tchar *name;	
{
	ENTRY	*h;

	if (any('/', name))
		return(COMMAND);

	h = hfind(name);

	if (h)
	{
		if (h->data & (BUILTIN | FUNCTION))
			return(h->data);
		else
			zapentry(h);
	}

	return(pathlook(name, 0, 0));
}


what_is_path(name)
	register tchar *name;			
{
	register ENTRY	*h;
	int		cnt;
	short	hashval;

	h = hfind(name);

	prst_buff(name);			
	if (h)
	{
		hashval = hashdata(h->data);

		switch (hashtype(h->data))
		{
			case BUILTIN:
				prs_buff((nl_msg(402, " is a shell builtin\n")));
				return;
	
			case FUNCTION:
			{
				struct tnamnod *n = lookup(name);  

				prs_buff((nl_msg(403, " is a function\n")));
				prst_buff(name);	
				prs_buff("(){\n");
				prf(n->namenv);
				prs_buff("\n}\n");
				return;
			}
	
			case REL_COMMAND:
			{
				short hash;

				if ((h->data & DOT_COMMAND) == DOT_COMMAND)
				{
					hash = pathlook(name, 0, 0);
					if (hashtype(hash) == NOTFOUND)
					{
						prs_buff((nl_msg(404, " not found\n")));
						return;
					}
					else
						hashval = hashdata(hash);
				}
			}

			case COMMAND:					
				prs_buff((nl_msg(405, " is hashed (")));
				pr_path(name, hashval);
				prs_buff(")\n");
				return;
		}
	}

	if (syslook(name, commands, no_commands))
	{
		prs_buff((nl_msg(406, " is a shell builtin\n")));
		return;
	}

	if ((cnt = findpath(name, 0)) > 0)
	{
		prs_buff((nl_msg(407, " is ")));
		pr_path(name, cnt);
		prc_buff(NL);
	}
	else
		prs_buff((nl_msg(404, " not found\n")));
}


findpath(name, oldpath)
	register tchar *name;
	int oldpath;
{
	register tchar 	*path;
	register int	count = 1;

	tchar	*p;	
	int	ok = 1;
	int 	e_code = 1;
	
	cost = 0;
	path = getpath(name);

	if (oldpath)
	{
		count = dotpath;
		while (--count)
			path = nextpath(path);

		if (oldpath > dotpath)
		{
			catpath(path, name);
			p = curstak();
			cost = 1;

			if ((ok = chk_access(p)) == 0)
				return(dotpath);
			else
				return(oldpath);
		}
		else 
			count = dotpath;
	}

	while (path)
	{
		path = catpath(path, name);
		cost++;
		p = curstak();

		if ((ok = chk_access(p)) == 0)
			break;
		else
			e_code = max(e_code, ok);

		count++;
	}

	return(ok ? -e_code : count);
}


chk_access(name)
	register tchar	*name;
{

	char *l_name = to_char(name);	/* local variable  NLS */
	if (access(l_name, EXECUTE) == 0)
		return(0);

	return(errno == EACCES ? 3 : 1);
}


pr_path(name, count)
	register tchar	*name;		
	int count;
{
	register tchar	*path;	

	path = getpath(name);

	while (--count && path)
		path = nextpath(path, name);

	catpath(path, name);
	prst_buff(curstak());
}


static
argpath(arg)
	register struct argnod	*arg;
{
	register tchar 	*s;
	register tchar	*start;

	while (arg)
	{
		s = arg->argval;
		start = s;

		if (letter(*s))		
		{
			while (alphanum(*s))
				s++;

			if (*s == '=')
			{
				*s = 0;

				if (eqtt(start, pathname))	
				{
					*s = '=';
					return(1);
				}
				else
					*s = '=';
			}
		}
		arg = arg->argnxt;
	}

	return(0);
}
