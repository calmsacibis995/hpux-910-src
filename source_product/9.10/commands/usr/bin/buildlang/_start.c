/* @(#) $Revision: 70.1 $ */

/*
 * This is a dummy routine.  It is called from crt0 just before
 * the call to main().  Its sole purpose is to allow another library the
 * chance to hook in their own version of _start() so that any
 * library initialization can be done transparently.
 */ 

_start()
{
}
