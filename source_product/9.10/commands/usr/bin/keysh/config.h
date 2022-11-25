/* RCS ID: @(#) $Header: config.h,v 66.4 90/07/13 11:03:31 rpt Exp $ */
/* Copyright (c) HP, 1989 */

#ifndef CONFIGincluded
#define CONFIGincluded

#define CONFIGbackup     TRUE
#define CONFIGinvisible  FALSE

extern void ConfigInitialize();

extern GlobalSoftKey *ConfigSoftKeyRoot(/* char *file */);

extern GlobalSoftKey *ConfigGetSoftKey(/* char *file, char *literal */);
extern GlobalSoftKey *ConfigGetOtherSoftKey(/* char *file, int backup */);

extern void ConfigPutSoftKeyBack(/* GlobalSoftKey *sk */);
extern void ConfigThrowSoftKeyAway(/* GlobalSoftKey *sk */);

extern void ConfigDefaultLabel(/* GlobalSoftKey *sk */);
extern void ConfigAssignAcceleratorsEtc(/* GlobalSoftKey *root */);

#endif
