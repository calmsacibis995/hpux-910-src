/* $Source: /misc/source_product/9.10/commands.rcs/lib/libc/csu/environ.c,v $
 * $Revision: 64.1 $
 */

#ifdef _NAMESPACE_CLEAN
#define environ _environ
#endif

#ifdef _NAMESPACE_CLEAN
#undef environ
#pragma _HP_SECONDARY_DEF _environ environ
#define environ _environ
#endif
char **environ = (void *) 0;
