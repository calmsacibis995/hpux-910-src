/* @(#) $Revision: 64.1 $ */       
#ifndef _MTAB_INCLUDED /* allow multiple inclusions */
#define _MTAB_INCLUDED

#ifdef __hp9000s800

/*
 * Mounted device accounting file.
 */
struct mtab {
	char	m_path[32];		/* mounted on pathname */
	char	m_dname[32];		/* block device pathname */
	char	m_type[4];		/* read-only, quotas */
};

#endif /* __hp9000s800 */
#endif /* _MTAB_INCLUDED */
