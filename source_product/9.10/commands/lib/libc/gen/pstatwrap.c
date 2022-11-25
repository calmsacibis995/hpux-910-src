/* @(#) $Revision: 70.2 $ */
/*LINTLIBRARY*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define pstat _pstat
#define pstat_getstatic _pstat_getstatic
#define pstat_getdynamic _pstat_getdynamic
#define pstat_getvminfo _pstat_getvminfo
#define pstat_getprocessor _pstat_getprocessor
#define pstat_getproc _pstat_getproc
#define pstat_getdisk _pstat_getdisk
#define pstat_getlv _pstat_getlv
#define pstat_getswap _pstat_getswap
#endif

#include <sys/param.h>
#include <sys/pstat.h>

/*  Lines added to cleanup ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef pstat_getstatic
#pragma _HP_SECONDARY_DEF _pstat_getstatic pstat_getstatic
#define pstat_getstatic _pstat_getstatic
#undef pstat_getdynamic
#pragma _HP_SECONDARY_DEF _pstat_getdynamic pstat_getdynamic
#define pstat_getdynamic _pstat_getdynamic
#undef pstat_getvminfo
#pragma _HP_SECONDARY_DEF _pstat_getvminfo pstat_getvminfo
#define pstat_getvminfo _pstat_getvminfo
#undef pstat_getprocessor
#pragma _HP_SECONDARY_DEF _pstat_getprocessor pstat_getprocessor
#define pstat_getprocessor _pstat_getprocessor
#undef pstat_getproc
#pragma _HP_SECONDARY_DEF _pstat_getproc pstat_getproc
#define pstat_getproc _pstat_getproc
#undef pstat_getdisk
#pragma _HP_SECONDARY_DEF _pstat_getdisk pstat_getdisk
#define pstat_getdisk _pstat_getdisk
#undef pstat_getlv
#pragma _HP_SECONDARY_DEF _pstat_getlv pstat_getlv
#define pstat_getlv _pstat_getlv
#undef pstat_getswap
#pragma _HP_SECONDARY_DEF _pstat_getswap pstat_getswap
#define pstat_getswap _pstat_getswap
#endif

extern int pstat();

/*
 * Get static system information
 */
int
pstat_getstatic(bufp, size, entries, offset)
struct pst_static *bufp;
size_t size;
size_t entries;
int offset;
{
	return pstat(PSTAT_STATIC, bufp, size, entries, offset);
}

/*
 * Get dynamic system information
 */
int
pstat_getdynamic(bufp, size, entries, offset)
struct pst_dynamic *bufp;
size_t size;
size_t entries;
int offset;
{
	return pstat(PSTAT_DYNAMIC, bufp, size, entries, offset);
}

/*
 * Get VM-related system information
 */
int
pstat_getvminfo(bufp, size, entries, offset)
struct pst_vminfo *bufp;
size_t size;
size_t entries;
int offset;
{
	return pstat(PSTAT_VMINFO, bufp, size, entries, offset);
}

/*
 * Get per-process information
 */
int
pstat_getproc(bufp, size, entries, offset)
struct pst_status *bufp;
size_t size;
size_t entries;
int offset;
{
	return pstat(PSTAT_PROC, bufp, size, entries, offset);
}

/*
 * Get Disk-related information
 */
int
pstat_getdisk(bufp, size, entries, offset)
struct pst_diskinfo *bufp;
size_t size;
size_t entries;
int offset;
{
	return pstat(PSTAT_DISKINFO, bufp, size, entries, offset);
}

/*
 * Get per-processor information
 */
int
pstat_getprocessor(bufp, size, entries, offset)
struct pst_processor *bufp;
size_t size;
size_t entries;
int offset;
{
	return pstat(PSTAT_PROCESSOR, bufp, size, entries, offset);
}

/*
 * Get per-logical volume information
 */
int
pstat_getlv(bufp, size, entries, offset)
struct pst_lvinfo *bufp;
size_t size;
size_t entries;
int offset;
{
	return pstat(PSTAT_LVINFO, bufp, size, entries, offset);
}

/*
 * Get per-swap area information
 */
int
pstat_getswap(bufp, size, entries, offset)
struct pst_swapinfo *bufp;
size_t size;
size_t entries;
int offset;
{
	return pstat(PSTAT_SWAPINFO, bufp, size, entries, offset);
}


