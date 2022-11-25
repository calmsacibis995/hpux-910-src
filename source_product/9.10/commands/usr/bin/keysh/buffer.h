/* RCS ID: @(#) $Header: buffer.h,v 66.3 91/01/01 15:08:42 rpt Exp $ */
/* Copyright (c) HP, 1989 */

#ifndef BUFFERincluded
#define BUFFERincluded

extern int bufferLine;
extern int bufferChar;

extern int BufferReadFd();
extern off_t BufferReadOffset();
extern int BufferPeekChar();
extern int BufferReadChar();

extern void BufferWriteFlush();
extern void BufferWriteChar();
extern void BufferWriteString();
extern int BufferWriteFd();

#endif
