/* @(#) $Revision: 66.2 $ */     

#ifdef libM
#define matherr _matherr
#endif /* libM */
int matherr()
{
	return 0;
}
