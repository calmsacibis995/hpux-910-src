/* @(#) $Revision: 66.2 $ */    

#ifndef _DEBUG_INCLUDED	/* allow multiple inclusions */
#define	_DEBUG_INCLUDED

#ifdef __hp9000s300

struct _debug_header
{
    long header_offset; /* file offset for xdb/cdb header table */
    long header_size;   /* length of xdb/cdb header table       */
    long gntt_offset;   /* file offset for GNTT                 */
    long gntt_size;     /* length of GNTT                       */
    long lntt_offset;   /* file offset for LNTT                 */
    long lntt_size;     /* length of LNTT                       */
    long slt_offset;    /* file offset for SLT                  */
    long slt_size;      /* length of SLT                        */
    long vt_offset;     /* file offset for VT                   */
    long vt_size;       /* length of VT                         */
    long xt_offset;     /* file offset for XT                   */
    long xt_size;       /* length of XT                         */
    long spare1;        /* for future expansion                 */
};

#endif /* __hp9000s300 */
#endif /* _DEBUG_INCLUDED */
