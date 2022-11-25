/* @(#) $Revision: 66.2 $ */    
#ifndef lint
#endif
#ifndef hpux
#endif

# include "defs"
# include "sys/types.h"
# include "sys/stat.h"

#ifdef NLS16
#include <nl_ctype.h>             /* NLS */
#endif

#ifdef VFORK                   /*added for 7.0 performance*/ 
#include <sys/fcntl.h>
int child_of_vfork = 0;        /*flag used by fatal*/
#endif

extern char Makecall;


/***************************/
/* MAIN FUNCTION           */
/***************************/
dosys(comstring, nohalt)
register CHARSTAR comstring;
int nohalt;
/****************************************************************/
{
    register CHARSTAR p;
    register int i;
    int status;

    p = comstring;
    while(    *p == BLANK ||
        *p == TAB) p++;
    if(!*p)
        return(-1);

    if(IS_ON(NOEX) && Makecall == NO)
        return(0);

    if(metas(comstring))
        status = doshell(comstring,nohalt);
    else
        status = doexec(comstring);

    return(status);
}



metas(s)  
register CHARSTAR s;
/****************************************************/
/* Determines if there are any shell metacharacters */
/* in the string.  Returns YES or NO only           */ 
/****************************************************/
{
#ifndef NLS16
    while(*s)
        if( funny[*s++] & META)
            return(YES);
#else
    if (s == NULL)   /* why isn't this check outside the ifdef? */
        return(NO);
    do {
        if (funny[*s] & META)
            return(YES);
    } while (CHARADV(s));          /* NLS: advance length of entire */
                                   /* char, even for 2-byte char's  */
#endif

    return(NO);
}


/* the VFORK ifdef was written by Steven Serocki.  Its purpose    */
/*   is to provide performance enhancement for the 7.0 timeframe. */
/*   With copy on write in the kernel for 8.0, this code will     */
/*   be rendered unnecessary.  The change was requested in order  */
/*   to provide faster performance by make in order to meet a     */
/*   contract obligation for the 800 in the 7.0 timeframe         */
#ifdef VFORK
doshell(comstring,nohalt)
register CHARSTAR comstring;
register int nohalt;
{
    register CHARSTAR shell;            /* make's shell definition */
    unsigned char expandshell[OUTMAX];  /* shell with expanded macros */

    if((waitpid = vfork()) == 0)    /* change waitpid to some other name */
    {
        child_of_vfork = 1;
        enbint(0);
            doclose();  /*modified for VFORK - sets close-on-exec flags*/
            setenv(); 
            shell = varptr("SHELL")->varval;
            if(shell == 0 || shell[0] == CNULL)
            	shell = SHELLCOM;

/****bug fix for FSDlj06171 ****/
            /* the SHELL variable may have macros in it which */
            /* need to be expanded before being given to execl */
            /* hence the call to subst here */
            subst(shell,expandshell,&expandshell[OUTMAX]);

            execl(expandshell, "sh", (nohalt ? "-c" : "-ce"), comstring, 0);
            fatal("Couldn't load Shell"); /* need to _exit */
    }
    /* if set in parent, a Real vfork was done */
        /* need to undo some state */
    /* the signal handler will be set to intrpt in await.  */
    if (child_of_vfork) {
        unsetenv();
        child_of_vfork=0;
    }
    return( await() );
}
#else
doshell(comstring,nohalt)
register CHARSTAR comstring;
register int nohalt;
/**************************************************************/
{
    register CHARSTAR shell;
    unsigned char expandshell[OUTMAX];  /* shell with expanded macros */

    if((waitpid = fork()) == 0)     /* change waitpid to some other name */
    {
        enbint(0);
        doclose();

        setenv();
        shell = varptr("SHELL")->varval;
        if(shell == 0 || shell[0] == CNULL)
            shell = SHELLCOM;

/****bug fix for FSDlj06171 ****/
        /* the SHELL variable may have macros in it which */
        /* need to be expanded before being given to execl */
        /* hence the call to subst here */
        subst(shell,expandshell,&expandshell[OUTMAX]);

        execl(expandshell, "sh", (nohalt ? "-c" : "-ce"), comstring, 0);
        fatal("Couldn't load Shell");
    }

    return( await() );
}
#endif



await()
/********************************************************/
{
    int intrupt();
    int status;
    int pid;

    if (waitpid == -1)               /* change waitpid to some other name */
        fatal("Unable to fork");
    enbint(intrupt);
    while( (pid = wait(&status)) != waitpid)
        if(pid == -1)
            fatal("bad wait code");
    waitpid = 0;
    return(status);
}






doclose()
/**********************************************/
/* Close open directory files before exec'ing */
/**********************************************/
{
    register OPENDIR od;

    for (od = firstod; od != 0; od = od->nextopendir)
        if (od->dirp != NULL) {
#ifdef VFORK
           /* clean up before the exec, as the file descriptors */
           /* were not copied in the vfork */
            if (fcntl(od->dirp->dd_fd, F_SETFD, 1) < 0)
            fatal("doclose: could not set close-on-exec-flag");
#else
            closedir(od->dirp);
#endif
        }
}





doexec(str)
register CHARSTAR str;
/****************************************************************/
{
    register CHARSTAR t;
    register CHARSTAR *p;
    CHARSTAR argv[OUTARGVMAX];
    int status;

    while( *str==BLANK || *str==TAB )
        ++str;
    if( *str == CNULL )
        return(-1);    /* no command */

    p = argv;
    for(t = str ; *t ; )
    {
        if (p < &argv[OUTARGVMAX-1])
            *p++ = t;
        else {
            fprintf(stderr,"make: Too many exec arguments.\n");
            exit(1);
        }
        while(*t!=BLANK && *t!=TAB && *t!=CNULL)
            ++t;
        if(*t)
            for( *t++ = CNULL ; *t==BLANK || *t==TAB  ; ++t);
    }

    *p = NULL;

/* the VFORK ifdef was written by Steven Serocki.  Its purpose    */
/*   is to provide performance enhancement for the 7.0 timeframe. */
/*   With copy on write in the kernel for 8.0, this code will     */
/*   be rendered unnecessary.  The change was requested in order  */
/*   to provide faster performance by make in order to meet a     */
/*   contract obligation for the 800 in the 7.0 timeframe         */

#ifdef VFORK
    if((waitpid = vfork()) == 0)
    {
            child_of_vfork = 1;
            enbint(0);
        doclose(); /*just set close-on-exec flag */
        setenv();
        execvp(str, argv);
        fatal1("Cannot load %s",str); /* will _exit */
    }
    /* if set in parent, a Real vfork happened */
        /* need to undo state */
    /* the signal handler will be set to intrpt in await.  */
    if (child_of_vfork) {
        unsetenv();
        child_of_vfork=0;
    }
#else
    if((waitpid = fork()) == 0)
    {
        enbint(0);
        doclose();
        setenv();
        execvp(str, argv);
        fatal1("Cannot load %s",str);
    }
#endif
    return( await() );
}



touch(force, name)
register int force;
register char *name;
/**********************************************************/
{
        struct stat stbuff;
        char junk[1];
        int fd;

        if( stat(name,&stbuff) < 0)
                if(force)
                        goto create;
                else
                {
                        fprintf(stderr,"touch: file %s does not exist.\n",name);
                        return;
                }
        if(stbuff.st_size == 0)
                goto create;
        if( (fd = open(name, 2)) < 0)
                goto bad;
        if( read(fd, junk, 1) < 1)
        {
                close(fd);
                goto bad;
        }
        lseek(fd, 0L, 0);
        if( write(fd, junk, 1) < 1 )
        {
                close(fd);
                goto bad;
        }
        close(fd);
        return;
bad:
        fprintf(stderr, "Cannot touch %s\n", name);
        return;
create:
        if( (fd = creat(name, 0666)) < 0)
                goto bad;
        close(fd);
}
