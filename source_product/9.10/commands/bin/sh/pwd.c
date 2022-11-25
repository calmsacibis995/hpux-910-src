/* @(#) $Revision: 70.1 $ */

/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include 		"defs.h"	/* added include for NLS */
#include		"mac.h"
#if defined(DUX) || defined(DISKLESS)
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef NLS
#define NL_SETN 1
#endif


#define	DOT		'.'
#define	NULL	0
#define	SLASH	'/'
#define MAXPWD  1024

extern char	longpwd[];

static tchar cwdname[MAXPWD];
static int 	didpwd = FALSE;

#if defined(DUX) || defined(DISKLESS)
cwd(dir2)
	register tchar *dir2;  /* This is needed because opendir()
                                  uses malloc() and dir2 is pointing
                                  to memory location obtained by sbrk()
                                  Yuck!! */
#else
cwd(dir)
	register tchar *dir;
#endif
{
	register tchar *pcwd;
	register tchar *pdir;
#if defined(DUX) || defined(DISKLESS)
        tchar dirbuf[1024];
        tchar *dir;
        tchar *find();
        tchar tmp[1024];
        struct stat tb;
        dir = dirbuf;
        tmovstr(dir2,dir);
#endif

	/* First remove extra /'s */

	rmslash(dir);

	/* Now remove any .'s */

	pdir = dir;
	while(*pdir) 			/* remove /./ by itself */
	{
                /* First make any /.+/ to /./ if .+ is really
                   a cdf */
#if defined(DUX) || defined(DISKLESS)
                if ((*pdir == DOT) && (*(pdir + 1) == '+') && (*(pdir + 2) == SLASH))
                {
                      tmp[0] = '\0';
                      movstrn(dir,tmp,pdir-dir);
                      tmp[pdir-dir] = '\0';
                      if (!(stat(to_char(tmp),&tb)<0) &&             /* stat OK */
                          ((tb.st_mode & S_IFMT) == S_IFDIR ) &&    /*directory */
                          (tb.st_mode & S_ISUID))            /*setuid bit on */
                            tmovstr(pdir+2,pdir+1);         /* chew up the '+' */
                }

#endif /* DUX || DISKLESS */
		if((*pdir==DOT) && (*(pdir+1)==SLASH))
		{
			tmovstr(pdir+2, pdir);
			continue;
		}
		pdir++;
		while ((*pdir) && (*pdir != SLASH))
			pdir++;
		if (*pdir)
			pdir++;
	}
	if(*(--pdir)==DOT && pdir>dir && *(--pdir)==SLASH)
		*pdir = NULL;


	/* Remove extra /'s */

	rmslash(dir);

	/* Now that the dir is canonicalized, process it */

	if(*dir==DOT && *(dir+1)==NULL)
	{
		return;
	}

	if(*dir==SLASH)
	{
		/* Absolute path */

		pcwd = cwdname;
		didpwd = TRUE;
	}
	else
	{
		/* Relative path */

		if (didpwd == FALSE)
			return;

		pcwd = cwdname + tlength(cwdname) - 1;
		if(pcwd != cwdname+1)
		{
			*pcwd++ = SLASH;
                        *pcwd = NULL;
		}
	}
#if defined(DUX) || defined(DISKLESS)
       /*if (pwd(-1) == 0) return; */
#endif /* DUX || DISKLESS */
	while(*dir)
	{

		if(*dir==DOT &&
		   *(dir+1)==DOT &&
		   (*(dir+2)==SLASH || *(dir+2)==NULL))
		{
			/* Parent directory, so backup one */

			if( pcwd > cwdname+2 )
				--pcwd;
			while(*(--pcwd) != SLASH)
				;
#if defined(DUX) || defined(DISKLESS)
                        tmovstr(find(cwdname,0),cwdname);
                        pcwd = tlength(cwdname)-1 + cwdname;
#else
			pcwd++;
#endif /* DUX || DISKLESS */
			dir += 2;
			if(*dir==SLASH)
			{
				dir++;
			}
#if defined(DUX) || defined(DISKLESS)
		        if((pcwd >= cwdname+1) &&
                           *(pcwd-1) != SLASH &&
                           *(dir))
		        {
				*pcwd++ = SLASH;
                        	*pcwd = NULL;
			}
#endif /* DUX || DISKLESS */
			continue;
		}
#if defined(DUX) || defined(DISKLESS)

		if(*dir==DOT &&
		   *(dir+1)==DOT &&
		   *(dir+2)=='+' &&
		   (*(dir+3)==SLASH || *(dir+3)==NULL))
		{
			/* Parent directory, so backup one */

			if( pcwd > cwdname+3 )
				--pcwd;
			while(*(--pcwd) != SLASH)
				;
                        tmovstr(find(cwdname,1),cwdname);
                        pcwd = tlength(cwdname)-1 + cwdname;
			dir += 3;
			if(*dir==SLASH)
			{
				dir++;
			}
		        if((pcwd >= cwdname+1) &&
                           *(pcwd-1) != SLASH &&
                           *(dir))
		        {
				*pcwd++ = SLASH;
                        	*pcwd = NULL;
			}
			continue;
		}
#endif /* DUX || DISKLESS */
		*pcwd++ = *dir++;
		while((*dir) && (*dir != SLASH))
			*pcwd++ = *dir++;
		if (*dir)
			*pcwd++ = *dir++;
                *pcwd = NULL;
	}
	*pcwd = NULL;

	--pcwd;
	if(pcwd>cwdname && *pcwd==SLASH)
	{
		/* Remove trailing / */

		*pcwd = NULL;
	}
	return;
}

/*
 *	Print the current working directory.
 */

#if defined(DUX) || defined(DISKLESS)
cwdprint(hopt)
int hopt;
#else
cwdprint()
#endif
{
#if defined(DUX) || defined(DISKLESS)
tchar tmp[1024];
        tmp[0] = 0;
        if (hopt) tmovstr(cwdname,tmp);
	pwd(hopt);
#else
	pwd();
#endif

	prst_buff(cwdname);
	prc_buff(NL);
#if defined(DUX) || defined(DISKLESS)
        if (hopt) tmovstr(tmp,cwdname);
#endif
	return;
}

/*
 *	This routine will remove repeated slashes from string.
 */

static
rmslash(string)
	tchar *string;
{
	register tchar *pstring;

	pstring = string;
	while(*pstring)
	{
		if(*pstring==SLASH && *(pstring+1)==SLASH)
		{
			/* Remove repeated SLASH's */

			tmovstr(pstring+1, pstring);
			continue;
		}
		pstring++;
	}

	--pstring;
	if(pstring>string && *pstring==SLASH)
	{
		/* Remove trailing / */

		*pstring = NULL;
	}
	return;
}

/*
 *	Find the current directory the hard way.
 */

#include	<ndir.h>
#if !defined(DUX) && !defined(DISKLESS)
#include	<sys/types.h>
#include	<sys/stat.h>
#endif


static char dotdots[] =
#if defined(DUX) || defined(DISKLESS)
"..//..//..//..//..//..//..//..//..//..//..//..//..//..//..//..//..//..//..//..//..//..//..//../";
#else
"../../../../../../../../../../../../../../../../../../../../../../../..";
#endif

extern DIR		*opendir();
extern struct direct	*readdir();
extern char		*cmovstrn();

static
#if defined(DUX) || defined(DISKLESS)
pwd(hopt)
#else
pwd()
#endif
{
	struct stat		cdir;	/* current directory status */
	struct stat		tdir;
	struct stat		pdir;	/* parent directory status */
	DIR			*pdirp;	/* parent directory pointer */

	struct direct		*dir;
#if defined(DUX) || defined(DISKLESS)
	char 			*dot = dotdots ;
	int			index = 1;
        char 			*nullptr = dotdots + 3;
        char                    *plus = dotdots+2;
        int matchcxt,matchprev,hidden,phidden,passback;
        char tmp[256];
#else
	char 			*dot = dotdots + sizeof(dotdots) - 3;
	int				index = sizeof(dotdots) - 2;
#endif
	int				cwdindex = MAXPWD - 1;
	int 			i;
#if defined(DUX) || defined(DISKLESS)
        struct stat dd;
        struct stat d;
        struct stat f;
        int cflag = 0;
#endif
#ifdef NLS
	char l_cwdname[MAXPWD];
#define cwdname l_cwdname
#endif /* NLS */
	cwdname[cwdindex] = 0;
	dotdots[index] = 0;

#if defined(DUX) || defined(DISKLESS)
        *nullptr = 0;
        passback = -1;
        phidden = 0;
        hidden = 0;
        matchcxt= matchprev=0;
        gcxt();
        if (hopt==-1) { hopt=0;cflag=1;} /* flag to return -1 */
#endif
	if(stat(dot, &pdir) < 0)
	{
#if defined(DUX) || defined(DISKLESS)
             	if (cflag)return(-1);
#endif
		error((nl_msg(701, "pwd: cannot stat .")));
	}
#if defined(DUX) || defined(DISKLESS)
        if (!(stat(".+",&f)<0)) phidden =1;
#endif

	dotdots[index] = '.';

	for(;;)
	{
		cdir = pdir;
#if defined(DUX) || defined(DISKLESS)
                passback++;
                hidden=phidden;
                *plus ='+';
                phidden = 0;
                if ((pdirp=opendir(dot))>0)
                {
                   /*if (!(fstat(pdirp->ff_fd,&pdir)<0) &&*/
                   if (!(stat(dot,&pdir)<0) &&
                        ((pdir.st_mode & S_IFMT) == S_IFDIR) &&
                        (pdir.st_mode & S_ISUID)){
                            phidden = 1;
                            goto START;
                   }
		   closedir(pdirp);
                }
                *plus = '/';
#endif
		if ((pdirp = opendir(dot)) == 0)
		{
#if defined(DUX) || defined(DISKLESS)
                	if (cflag)return(-1);
#endif
			error((nl_msg(702, "pwd: cannot open ..")));
		}

#if defined(DUX) || defined(DISKLESS)
		if(stat(dot, &pdir) < 0) /* fstat is buggy*/
#else
		if(fstat(pdirp->dd_fd, &pdir) < 0)
#endif
		{
			closedir(pdirp);
#if defined(DUX) || defined(DISKLESS)
             	if (cflag)return(-1);
#endif
			error((nl_msg(701, "pwd: cannot stat ..")));
		}
#if defined(DUX) || defined(DISKLESS)
START:
#endif

#ifdef RFA
		/*
		 * Fix RFA loop-back .. problem for FSDlj04034
		 */
		if (cdir.st_dev == pdir.st_dev &&
                    cdir.st_netino == pdir.st_netino)
#else
		if (cdir.st_dev == pdir.st_dev)
#endif /* RFA */
		{
#if defined(DUX) || defined(DISKLESS)
			if ((cdir.st_ino == pdir.st_ino) &&
                            (cdir.st_cnode == pdir.st_cnode))
#else
			if (cdir.st_ino == pdir.st_ino)
#endif

			{
				didpwd = TRUE;
				closedir(pdirp);
				if (cwdindex == (MAXPWD - 1))
					cwdname[--cwdindex] = SLASH;

				movstr(&cwdname[cwdindex], cwdname);
#ifdef NLS
#undef cwdname
 				sto_tchar(l_cwdname, cwdname);
#define cwdname l_cwdname
#endif /* NLS */
#if defined(DUX) || defined(DISKLESS)
                                /*if (cdir.st_remote){
                                     get_net_name(&cdir);
                                }*/
				return(0);
#else
				return;
#endif
			}

			do
			{
				if ((dir = readdir(pdirp)) == 0)
				{
					closedir(pdirp);
#if defined(DUX) || defined(DISKLESS)
             	if (cflag)return(-1);
#endif
					error((nl_msg(703, "pwd: read error in ..")));
				}
			}
			while (dir->d_ino != cdir.st_ino);
		}
		else
		{
			char name[MAXPATHLEN];

			movstr(dot, name);
			i = length(name) - 1;

			name[i++] = '/';

			do
			{
				if ((dir = readdir(pdirp)) == 0)
				{
					closedir(pdirp);
#if defined(DUX) || defined(DISKLESS)
             	if (cflag)return(-1);
#endif
					error((nl_msg(703, "pwd: read error in ..")));
				}
				*(cmovstrn(dir->d_name, &name[i], MAXNAMLEN)) = 0;
				stat(name, &tdir);
			}
			while (tdir.st_ino != cdir.st_ino
			       || tdir.st_dev != cdir.st_dev
#if defined(DUX) || defined(DISKLESS)
			       || tdir.st_cnode != cdir.st_cnode
#endif /* DUX || DISKLESS */
#ifdef RFA
			       /*
				* Fix RFA loop-back .. problem
				* FSDlj04034
				*/
			       || tdir.st_netino != cdir.st_netino
#endif /* RFA */
			);
		}

		i = dir->d_namlen;
#if defined(DUX) || defined(DISKLESS)
                strcpy(tmp,dir->d_name);
                strcat(tmp,"+");
                matchcxt=match(tmp) || match(dir->d_name);
                if ((!hopt && passback && hidden && matchprev && matchcxt) ||
                    (!hopt && phidden && !hidden && matchcxt)){
                     i=0;
                     *(dir->d_name) = 0;
                     matchprev = passback = 1;
                     goto DONEWR;
                }else if ((hidden && hopt && passback)
                      || (hidden && !passback)
                      || (passback && hidden && !matchprev)
                      && !(matchprev && !phidden && passback && !hopt &&
                           !matchcxt))
               {
                   i++;
                   strcat(dir->d_name,"+");
               }
                   matchprev = ( matchcxt && !passback && !hidden) ||
                               (matchcxt && matchprev);
#endif
		closedir(pdirp);

		if (i > cwdindex - 1)
				error(nl_msg(635,longpwd));
		else
		{
			cwdindex -= i;
			cmovstrn(dir->d_name, &cwdname[cwdindex], i);
			cwdname[--cwdindex] = SLASH;
		}

#if defined(DUX) || defined(DISKLESS)
DONEWR:
                *nullptr = '/';
                plus = plus+4;
                nullptr=nullptr+4;
                *(nullptr) = 0;

#else
		dot -= 3;
#endif
#if defined(DUX) || defined(DISKLESS)
                if(dot > (&dotdots[0] + strlen(dotdots) -1)){
#else
		if (dot<dotdots)
#endif
#if defined(DUX) || defined(DISKLESS)
             	if (cflag)return(-1);
#endif
			error(nl_msg(635,longpwd));
#if defined(DUX) || defined(DISKLESS)
                }
#endif
	}
}


#if defined(DUX) || defined(DISKLESS)
/* Code to deal with mismatched context */
char *context[20];
int cxt;
gcxt()
{
   int i,k;
   char buf[512];
   char str[120];
   buf[0] = '\0';
   getcontext(buf,512,1);
   i=0;
   cxt=0;
   while (buf[i] != '\0' && i<512){
                  while (buf[i] == ' ' && i < 512) i++;
                  k = 0;
		  while (buf[i] != ' ' && buf[i] != '\0' && i < 512) {
                           str[k] = buf[i];
                           i++;k++;
                  }
                  str[k] = '\0';
                  context[cxt] = (char *)malloc(strlen(str)+1);
                  strcpy(context[cxt],str);
                  cxt++;
   }
}
int match(str)
char *str;
{
  int i;
   i = 0;
   while ( i < cxt ) {
                if (!strcmp(context[i],str)) return(1);
                i++;
   }
   return(0);
}



#define DOTDOT 0
#define DOTDOTP 1
tchar destbuf[1024];
tchar ftmp2[1024];
tchar ftmp[1024];
tchar *Strcpy(),*Strchr(),*Strrchr(),*Strcat();
struct stat dest;
tchar *find(path,pform)
tchar *path;
int pform;
{
  tchar save;
  tchar *ptr1,*ptr2;
  struct stat t,t2;
  int hidden;
  extern match_inode();
  char getcdfbuf[1024];


  Strcpy(destbuf,path);
  if (*(path + tlength(path)-2) != '/')Strcat(ftmp,to_tchar("/"));
  if (!pform) Strcat(destbuf,to_tchar(".."));
  else Strcat(destbuf,to_tchar("..+"));
  if(stat(to_char(destbuf),&dest) <0)return(NULL);
  Strcpy(ftmp2,path);
  /* Quick check for root */
  if ( (!stat("/",&t)) && (t.st_ino == dest.st_ino) &&
       (t.st_dev == dest.st_dev ) && (t.st_cnode == dest.st_cnode ) ){
            tmovstr(to_tchar("/"),ftmp);
            return(ftmp);
  }
  /* Quick check for same inode */
  if ( (!stat(ftmp2,&t)) && (t.st_ino == dest.st_ino) &&
       (t.st_dev == dest.st_dev ) && (t.st_cnode == dest.st_cnode ) ){
            return(ftmp2);
  }



  switch(pform){

  case DOTDOT:
                /* dest path must already be embedded in path */
                ptr2=ptr1=Strchr(ftmp2,'/');
                while (*ptr1 ){
                	while (*ptr1 && *ptr1 == '/')ptr1++;
                	while (*ptr1 && *ptr1 != '/'){
                       		ptr1++;
                	}
                	save = *ptr1;
                	*ptr1=NULL;
                	stat(to_char(ftmp2),&t2);   /* assume path is legal */
                	if (t2.st_ino == dest.st_ino &&
                    		t2.st_cnode == dest.st_cnode &&
                    		t2.st_dev == dest.st_dev ){
                      			return(ftmp2);
                	}else{
                       			*ptr1 = save;
                       			if (save == NULL) break;
                       			ptr1++;
                	}
                }
                /* we exhaust the buffer and can't find a matching inode
                   just back up one component */

                /* '/' special case */
                ptr1 = (ftmp2 + tlength(ftmp2) -2);
                if (ptr1 == ftmp2 && *ptr1 == '/') return(ftmp2);
                else if (*ptr1 == '/'){    /* Trailing '/' */
                            *ptr1 = NULL;
                }

                ptr1=Strrchr(ftmp2,'/');
                *ptr1= NULL;
                return(ftmp2);
                break;
  case DOTDOTP:
                /* dot dot plus */

                stat(to_char(ftmp2),&t2);
                if (((t2.st_mode & S_IFMT) == S_IFDIR) &&
                    (t2.st_mode & S_ISUID)){
                        hidden = 1;
                }else hidden = 0;

                /* '/' special case */
                ptr1 = (ftmp2 + tlength(ftmp2) -2);
                if (ptr1 == ftmp2 && *ptr1 == '/') return(ftmp2);
                else if (*ptr1 == '/'){    /* Trailing '/' */
                            *ptr1 = NULL;
                }

                /* get last component */
                ptr1 = Strrchr(ftmp2,'/');
                if (hidden) {
                        *ptr1=NULL;
                         return(ftmp2);
                }else{
                        /*ptr1=NULL;*/
                        ftmp[0] = NULL;
                        getcdfbuf[0] = NULL;
                        getcdf(to_char(ftmp2),getcdfbuf,1024);
                        if ( getcdfbuf[0] != '\0')
                                 tmovstr(to_tchar(getcdfbuf),ftmp);
                        else
                                 tmovstr(ftmp2,ftmp);
                        ptr1=Strrchr(ftmp,'/');
                        *ptr1 =NULL ;
                        return(ftmp);
                }
                break;
   }
}
tchar *Strchr(s,c)
tchar c,*s;
{
   tchar d;
   while (d = *s++)
   {
          if (d == c) return(s--);
   }
   return(NULL);
}
tchar *Strrchr(s,c)
tchar c,*s;
{
   tchar d;
   tchar *p;
   tchar null;
   p = &null;
   *p=NULL;
   while (d = *s++)
   {
          if (d == c) {
                        p=s-1;
          }
   }
   if (*p == NULL) return(s--);
   return(p);
}
tchar *Strcpy(a,b)
tchar *a,*b;
{
   return(tmovstr(b,a));
}
tchar *Strcat(a,b)
tchar *a,*b;
{
  return(tmovstr(b,a+tlength(a)-1));
}
#endif /* DUX || DISKLESS */
