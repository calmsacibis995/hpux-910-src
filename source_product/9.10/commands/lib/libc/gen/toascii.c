/* @(#) $Revision: 64.2 $ */   
/*LINTLIBRARY*/


#undef toascii

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _toascii toascii
#define toascii _toascii
#endif

int 
toascii(c)
register int c;
{
    return (c&0177);
}
