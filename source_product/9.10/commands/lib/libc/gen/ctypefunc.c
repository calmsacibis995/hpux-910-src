/* @(#) $Revision: 70.1 $ */     
/*LINTLIBRARY*/
/*
 * ctype function implementation of the macros isalnum, isalpha, isascii,
 * iscntrl, isdigit, isgraph, islower, isprint, ispunct, isspace,
 * isupper and isxdigit
*/
#ifdef _NAMESPACE_CLEAN
# ifdef __lint
# define isalnum  _isalnum 
# define isalpha  _isalpha 
# define isascii  _isascii 
# define iscntrl  _iscntrl 
# define isdigit  _isdigit 
# define isgraph  _isgraph 
# define islower  _islower 
# define isprint  _isprint 
# define ispunct  _ispunct 
# define isspace  _isspace 
# define isupper  _isupper 
# define isxdigit _isxdigit 
# endif /* __lint */
#endif

#include <ctype.h>

#undef isalnum

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _isalnum isalnum
#define isalnum _isalnum
#endif /* _NAMESPACE_CLEAN */

int
isalnum(c)
int c;
{
    return((__ctype2[c]&(_A))|(__ctype[c]&(_N))); /* was (_U|_L|_N)) */
}


#undef isalpha

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _isalpha isalpha
#define isalpha _isalpha
#endif /* _NAMESPACE_CLEAN */

int
isalpha(c)
int c;
{
    return(__ctype2[c]&(_A)); /* was ctype...(_U|_L) */
}


#undef isascii

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _isascii isascii
#define isascii _isascii
#endif

int
isascii(c)
int c;
{
    return((unsigned) (c) <= 0177);
}


#undef iscntrl

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _iscntrl iscntrl
#define iscntrl _iscntrl
#endif /* _NAMESPACE_CLEAN */

int
iscntrl(c)
int c;
{
    return(__ctype[c]&_C);
}


#undef isdigit

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _isdigit isdigit
#define isdigit _isdigit
#endif /* _NAMESPACE_CLEAN */

int
isdigit(c)
int c;
{
    return(__ctype[c]&_N);
}


#undef isgraph

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _isgraph isgraph
#define isgraph _isgraph
#endif /* _NAMESPACE_CLEAN */

int
isgraph(c)
int c;
{
    return(__ctype2[c]&(_G)); /* was (_P|_U|_L|_N) */
}


#undef islower

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _islower islower
#define islower _islower
#endif /* _NAMESPACE_CLEAN */

int
islower(c)
int c;
{
    return(__ctype[c]&_L); 
}


#undef isprint

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _isprint isprint
#define isprint _isprint
#endif /* _NAMESPACE_CLEAN */

int
isprint(c)
int c;
{
    return(__ctype2[c]&(_PR));/* was ctype...(_P|_U|_L|_N|_B)) */
}


#undef ispunct

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _ispunct ispunct
#define ispunct _ispunct
#endif /* _NAMESPACE_CLEAN */

int
ispunct(c)
int c;
{
    return(__ctype[c]&_P); 
}


#undef isspace

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _isspace isspace
#define isspace _isspace
#endif /* _NAMESPACE_CLEAN */

int
isspace(c)
int c;
{
    return(__ctype[c]&_S); 
}


#undef isupper

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _isupper isupper
#define isupper _isupper
#endif /* _NAMESPACE_CLEAN */

int
isupper(c)
int c;
{
    return(__ctype[c]&_U); 
}


#undef isxdigit

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _isxdigit isxdigit
#define isxdigit _isxdigit
#endif /* _NAMESPACE_CLEAN */

int
isxdigit(c)
int c;
{
    return(__ctype[c]&_X); 
}
