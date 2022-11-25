/* @(#) $Revision: 27.1 $ */      
#ifndef DIRSIZ
#define DIRSIZ 14
#endif

struct	direct
{
	short	d_ino;
	char	d_name[DIRSIZ];
};
