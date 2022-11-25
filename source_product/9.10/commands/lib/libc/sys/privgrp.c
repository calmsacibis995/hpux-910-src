/* @(#) $Revision: 64.2 $ */      
#ifdef _NAMESPACE_CLEAN
#define getprivgrp _getprivgrp
#define syscall _syscall
#define setprivgrp _setprivgrp
#endif

#include <sys/types.h>
#include <sys/privgrp.h>

#define PRIVSYS	151

#define GETPRIVGRP	0
#define SETPRIVGRP	1

#ifdef _NAMESPACE_CLEAN
#undef getprivgrp
#pragma _HP_SECONDARY_DEF _getprivgrp getprivgrp
#define getprivgrp _getprivgrp
#endif

getprivgrp(grplist)
struct privgrp_map *grplist;
{
	return(syscall(PRIVSYS, GETPRIVGRP, grplist));
}

#ifdef _NAMESPACE_CLEAN
#undef setprivgrp
#pragma _HP_SECONDARY_DEF _setprivgrp setprivgrp
#define setprivgrp _setprivgrp
#endif
setprivgrp(grpid, mask)
int grpid, *mask;
{
	return(syscall(PRIVSYS, SETPRIVGRP, grpid, mask));
}

