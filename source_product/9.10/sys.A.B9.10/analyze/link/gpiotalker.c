/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/link/RCS/gpiotalker.c,v $
 * $Revision: 1.10.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:27:22 $
 *
/* 
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/


#ifdef BOBCAT

/*
 *  The following construct will allow `what' to identify each
 *  source module.  MODULE_ID should be undefined before you
 *  submit your product to manufacturing and should be defined
 *  during development and test.
 */

#ifdef MODULE_ID
static char ModuleId[] = "@(#) $Header: gpiotalker.c,v 1.10.83.3 93/09/17 16:27:22 root Exp $";
#endif MODULE_ID


#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 1	/* set number */
/*
 *   Remember the -I cc argument redirects the default
 *   for the include files.
 */
#include "/usr/include/msgbuf.h"
#endif NLS

#include "/usr/include/stdio.h"
#include "/usr/include/errno.h"

#include   "/usr/include/sys/types.h"
#ifndef INTERNAL5.1
#include   "/usr/include/sys/hpib.h"
#endif NOT INTERNAL5.1
#include   "/usr/include/sys/gpio.h"
#include   "/usr/include/sys/dilio.h"
#include   "/usr/include/sys/dil.h"
#include   "/usr/include/signal.h"
#include   "/usr/include/time.h"

#include   "ddbCons.h"
#include   "ddbStd.h"

#define   STI0  0x01
#define   STI1  0x02

#define GP_CTL0 0x01
#define GP_CTL1 0x02

#ifdef INTERNAL5.1
#undef HPIB_BUS_STATUS
#undef HPIB_STATUS
#undef HPIB_SRQ
#undef HPIB_CONTROL
#define HPIB_BUS_STATUS STATUS_LINES
#define HPIB_STATUS GPIO_STATUS
#define HPIB_SRQ CONTROL_LINES
#define HPIB_CONTROL  GPIO_CONTROL
#endif INTERNAL5.1

#define BOOTINDIGO  3
#define READ        5
#define WRITE       4
#define INTRUPT     6

#define TIMEOUTLIMIT   10   /* timeout is 10 seconds */
#define NEVERTIMEOUT   0

#define BUFSIZE     1000000
#define SIGINTMASK   (1L << (SIGINT - 1))


/* IMPORTS */
IMPORT   unsigned int   hitCtrlC;
IMPORT   int           verboseErrors;
IMPORT   char           *myname;

/* EXPORTS */

#ifdef ERRORTEST
    int ddbErrNumber;
#endif ERRORTEST

/* LOCAL Variables */
 static char errBuf[100];
 static int errnoSave;
 static struct itimerval vttimer, old_vttimer, zerotimer=0;
 static struct itimerval get_vttimer;
 static int timeout;     /* global timeout flag */

/*------------------------------------------------------------------------
 * XXXioctl  (GpioIoctl) - This process translates DDB control requests
 * into requests to the low level DDB I/O interface routines.
 *
 * The only requests supported are:
 *   1.  INTERRUPT - send interrupt to target system.  Tell the target
 *                   system to stop executing and transfer control to
 *                   kernel rdb code.
 *   2.  AWRITE   - This is a write into the continue flag in the target.
 *                  This will cause the kernel rdb code to return control
 *                  to the process ddb interrupted.
 *   3.  BOOT     - boot the target system.  This instructs the ISL rdb
 *                  routine to transfer control to the address provided.
 *
 * Upon entry to this process a `sigblock' request is issued to block
 * delivery of an interrupt signal.  This allows the `driver'
 * code for ioctl to run uninterrupted.  Just before exit signal processing
 * is set back to its original state.
 *
 * CALL SEQUENCE:  GpioIoctl(cfd,cnmd,args)
 *       where:  cfd  = comm link file descriptor
 *               cnmd = the command to preform (see above)
 *               args = arguments associated with each command
 *
 * RETURN VALUES: return(SUCCESS/FAIL)
 *
 * GLOBALS: verboseErrors
 *
 * XXXioctl  (GpioIoctl)  is called from:  boot_target (ddbHi.c)
 *                                         run_target  (ddbRun.c)
 *                                         send_stop   (ddbHi.c)
 *
 * GpioIoctl calls: handshakeEIR, send_command, spec_wr
 *
 * system routine(s) used: fprintf, nl_msg, sigblock, sigsetmask
 *----------------------------------------------------------------------*/
int GpioIoctl (cfd, command, args)
int   cfd;      /* file descripter */
int   command;
int   args[];      /* arguments */
{
   register int result = 0, sigmask;

#ifdef RDB
   /*  Block delivery of signals */
   sigmask = sigblock (SIGINTMASK);
#endif RDB

   switch (command) 
   {
      case INTERRUPT:
      case AWRITE:
      {
         unsigned int offset;
         char *buf;
         int len, stoprunning;
   
#ifdef DDBDEBUG
         fprintf(stderr,"GpioIoctl: INTERRUPT or AWRITE\n");
#endif DDBDEBUG

         buf      = (char *)args[1];
         offset   = args[2];
         len      = args[3];
	 /*
	  *  Unfortunately, lower level procedures must be told
	  *  if this request is the result of an XDB quit
	  *  command.  The handshaking they do is different if
	  *  it is.
	  */
         stoprunning = args[4];
   
	 /*
	  * spec_wr manages the 'handshake' for AWRITE & INTERRUPT
	  */
         result = spec_wr(cfd,buf,offset,len,command,stoprunning);
#ifdef ERRORTEST
         if ((result == FAIL) || (ddbErrTest(1) == TRUE))
#else
         if (result == FAIL)
#endif ERRORTEST
         {
            if (verboseErrors == YES)
               fprintf(stderr, (nl_msg(1278, "spec_wr(%d,buf,%x,%d,%d,%d) failed, at (1)\n")),
                                          cfd,offset,len,command,stoprunning);
	    return(FAIL);
         }
      }

      /*  OK, go return */
      break;

      /*------------------------------------------------------------*/

      case BOOT:

#ifdef DDBDEBUG
         fprintf(stderr,"GpioIoctl (BOOT): calling send_command\n");
#endif DDBDEBUG

         /*
          *  args[0] conatins the address in the target of where
          *  the ISL rdb routine is to transfer control.
          */
         result = send_command(cfd,args[0],0,BOOTINDIGO);
#ifdef ERRORTEST
         if ((result == FAIL) || (ddbErrTest(2) == TRUE))
#else
         if (result == FAIL)
#endif ERRORTEST
         {
            if (verboseErrors == YES)
               fprintf(stderr, (nl_msg(1280, "send_command(%d,%x,0,%d) failed, at (1)\n")),
                                                 cfd,args[0],BOOTINDIGO);
            return(FAIL);
         }

#ifdef DDBDEBUG
         fprintf(stderr,"GpioIoctl (BOOT): calling handshake\n");
#endif DDBDEBUG

/*
 *  The target rdb code raises EIR twice on a boot operation.
 *  The last one indicates that the target is in command mode and
 *    is ready to receive commands.  I don't remember why the
 *    first one occurs.  But, it does..
 */

         result = handshakeEIR(cfd,TIMEOUTLIMIT);
#ifdef ERRORTEST
         if ((result == FAIL) || (ddbErrTest(3) == TRUE))
#else
         if (result == FAIL)
#endif ERRORTEST
         {
            if (verboseErrors == YES)
               fprintf(stderr, (nl_msg(1282, "handshakeEIR(%d,%d) failed, at (1)\n")),
                                                  cfd,TIMEOUTLIMIT);
            return(FAIL);
         }
         
#ifdef DDBDEBUG
         fprintf(stderr,"GpioIoctl (BOOT): calling handshake\n");
#endif DDBDEBUG

         result = handshakeEIR(cfd,NEVERTIMEOUT);
#ifdef ERRORTEST
         if ((result == FAIL) || (ddbErrTest(4) == TRUE))
#else
         if (result == FAIL)
#endif ERRORTEST
         {
            if (verboseErrors == YES)
               fprintf(stderr, (nl_msg(1284, "handshakeEIR(%d,%d) failed, at (2)\n")),
                                                 cfd,NEVERTIMEOUT);
            return(FAIL);
         }
         
         break;
   
      /*------------------------------------------------------------*/

      /*  Ohhhh Noooo.  I don't know this request   */
      default:
         fprintf(stderr, (nl_msg(1285, "Unimplemented gpioioctl call %d\n")), command);
         return(FAIL);
         break;
   }

#ifdef RDB
   /*  Turn signals back on */
   sigsetmask (sigmask);
#endif RDB

#ifdef DDBDEBUG
   fprintf(stderr,"GpioIoctl: returning\n");
#endif DDBDEBUG

   return(SUCCESS);
}

/*------------------------------------------------------------------------
 *
 *  XXXread (GpioRead) - This process manages the operation necessary to
 *  process the read request.
 *
 *  We first block the interrupt signal (SIGINT).  Then call the read
 *  routine (gpio_rd).  When the transfer completes the interrupt signal
 *  is reenabled.
 *
 *  CALL SEQUENCE: GpioRead(cfd,addr,buffer,len)
 *        where: cfd  = comm link file descriptor
 *               addr = addr in target of where to start reading
 *               len  = number of bytes to transfer
 *
 *  RETURN VALUES: return(results/FAIL)
 *        where: results = number of bytes actually transfered
 *             buffer  = len bytes starting at addr in target system
 *
 *  GLOBALS: verboseErrors
 *
 *  XXXread (GpioRead)  is called from:  rmRead (ddbRdWr.c)
 *
 *  GpioRead calls: gpio_rd
 *
 *  system routine(s) used: fprintf, nl_nsg, sigblock, sigsetmask
 *----------------------------------------------------------------------*/
int GpioRead (cfd, address, buffer, length)
int    cfd;       /* file descripter */
long   address;   /* offset from origin */
char   *buffer;   /* buffer address */
long   length;    /* number of bytes to read */
{
   register int result, sigmask;

#ifdef RDB
   /*  Block delivery of signals  */
   sigmask = sigblock (SIGINTMASK);
#endif RDB

   result = gpio_rd (cfd, address, buffer, length);
#ifdef ERRORTEST
         if ((result == FAIL) || (ddbErrTest(5) == TRUE))
#else
   if (result == FAIL)
#endif ERRORTEST
      if (verboseErrors == YES)
         fprintf(stderr, (nl_msg(1287, "gpio_rd(%d,%x,buffer,%d) failed, at (1)\n")),
                                            cfd,address,length,READ);
   
#ifdef RDB
   /*  Turn delivery of signals back on  */
   sigsetmask (sigmask);
#endif RDB

   return result;
}


/*------------------------------------------------------------------------
 *
 *  XXXwrite (GpioWrite) - This process manages the operation 
 *  necessary to process the write request.
 *
 *  We first block the interrupt signal (SIGINT).  Then call the write
 *  routine (gpio_wr).  When the transfer completes the interrupt signal
 *  is reenabled.
 *
 *  CALL SEQUENCE: GpioWrite(cfd,addr,buffer,len)
 *        where: cfd    = comm link file descriptor
 *               addr   = addr in target of where to start I/O
 *               buffer = pointer to data buffer to transfer
 *               len    = number of bytes to transfer
 *
 *  RETURN VALUES: return(results/FAIL)
 *        where: results = number of bytes actually transfered
 *
 *  GLOBALS: verboseErrors
 *
 *  XXXwrite (GpioWrite) is called from:  rmWrite (ddbHi.c)
 *
 *  GpioWrite calls: gpio_wr
 *
 *  system routine(s) used: fprintf, sigblock, sigsetmask
 *----------------------------------------------------------------------*/
int GpioWrite (cfd, address, buffer, length)
int    cfd;       /* file descripter */
long   address;   /* offset from origin */
char   *buffer;   /* buffer address */
long   length;    /* number of bytes to write */
{
   register int result, sigmask;

#ifdef RDB
   /*  Block delivery of signals  */
   sigmask = sigblock (SIGINTMASK);
#endif RDB

   result = gpio_wr (cfd, buffer, length, address, WRITE, NO);
#ifdef ERRORTEST
         if ((result == FAIL) || (ddbErrTest(6) == TRUE))
#else
   if (result == FAIL)
#endif ERRORTEST
      if (verboseErrors == YES)
         fprintf(stderr, (nl_msg(1288, "gpio_wr(%d,buffer,%d,%x,%d,%d) failed, at (2)\n")),
                                    cfd,length,address,WRITE,NO);
   
#ifdef RDB
   /*  Turn delivery of signals back on  */
   sigsetmask (sigmask);
#endif RDB

   return(result);
}

/*#########################################################################*/


/*------------------------------------------------------------------------
 * gpio_rd - This process controls reading from the target.
 *
 *   1.  Verify the requested length will fit in the local read buffer.
 *       The original 9920 based driver required the target system send 
 *       two (2) bytes more than requested.  Since we `must' follow this
 *       established protocol we need to read two more bytes than the 
 *       user asked for.  That's why we have a local buffer.  It gives us
 *       a place to read the extra two bytes.
 *
 *   2.  Send the READ command packet to the target to tell it we want
 *       data sent to us.
 *
 *   3.  Read the data
 *
 *   4.  Hand shake EIR (wait for the target to configure for a new
 *       command packet).
 *
 *   5.  Copy the local buffer to the user's buffer.
 *
 * CALL SEQUENCE: gpio_rd(file,address,buffer,length)
 *       where: file    = comm link file descriptor
 *              address = addr in target of where to start reading
 *              length  = number bytes user wants transfered
 *
 * RETURN VALUES: return(results/FAIL)
 *       where: results = number of bytes actually transfered
 *              buffer  = length bytes starting at address in target
 *
 * GLOBALS: errno (errno.h), errnoSave, errBuf
 *
 * gpio_rd is called from:  GpioRead
 *
 * gpio_rd calls: handshakeEIR, send_command
 *
 * system routine(s) used: fprintf, nl_msg, perror, read, sprintf
 *----------------------------------------------------------------------*/
static int gpio_rd (cfd, address, buffer, length)
int    cfd;            /* file descripter */
char   *buffer;        /* buffer address */
long   length;         /* number of bytes to read */
unsigned int address;  /* address to read or write from */
{
   register unsigned int *from, *to;
   register int alen, result, action;
   register int i;
   static char   localBuffer [BUFSIZE];
   int errorflag;
   struct cmdRec 
   {
      unsigned int command, address, length;
   } cmd;


/*
 *  Will the data fit in the local buffer???
 */
#ifdef ERRORTEST
   if ((length+2 > BUFSIZE) || (ddbErrTest(7) == TRUE))
#else
   if (length+2 > BUFSIZE) 
#endif ERRORTEST
   {
      fprintf(stderr, (nl_msg(1289, "Length (%d) in GPIO read request > %d, at (1)\n")),
			 length+2,BUFSIZE);
      return(FAIL);
   }

#ifdef DDBDEBUG
   fprintf(stderr,"GPIO_RD: calling send_command\n");
#endif DDBDEBUG

   /*  Send the command packet  */
   result = send_command(cfd,address,length,READ);
#ifdef ERRORTEST
   if ((result == FAIL) || (ddbErrTest(8) == TRUE))
#else
   if (result == FAIL)
#endif ERRORTEST
   {
      if (verboseErrors == YES)
         fprintf(stderr, (nl_msg(1291, "send_command(%d,%x,%d,%d) failed, at (2)\n")),
                                              cfd,address,length,READ);
      return(FAIL);
   }

#ifdef DDBDEBUG
   fprintf(stderr,"GPIO_RD: issuing read\n");
#endif DDBDEBUG

   /*
    *  We read two bytes more than the user requested because 
    *  that's the way the protocol acts.  For reasons I don't
    *  know the 9920 rdb driver needed the target to send two
    *  extra bytes.  Well the target still sends them and xdb
    *  doesn't want them.
    *
    *  The target is now 'waiting' for our GPIO card to indicate
    *  that it is configured to read.  There is a signal line
    *  which tells the target we're ready to receive.  As soon as 
    *  target sees this line go logically high it will start
    *  sending data.
    */
repeatread:
   alen = read (cfd, localBuffer, length+2);
#ifdef ERRORTEST
   if ((alen != length+2) || (ddbErrTest(9) == TRUE))
#else
   if (alen != length+2)
#endif ERRORTEST
   {
        errnoSave=errno;
      /*
       * This kludge is because of a bug in the 300 kernel.
       * The kernel is counting the number and size of transfers
       * to the file.  This is ok, for real disc file and a bug
       * for device files.  We must close the file and the reopen
       * it to reset the counter.
       */
      errorflag=FALSE;
      if ((errno=22) && errorflag)
      {
	 errorflag=TRUE;
         if (get_comm_link(&cfd,'\0') == FAIL) 
         {
           if (verboseErrors == YES)
              fprintf(stderr, (nl_msg(1093, "get_comm_link(&cfd,NULL) failed\n")));
           return(FAIL);
         }
         goto repeatread;
      }
        sprintf(errBuf,"%s",myname);
        fprintf(stderr, (nl_msg(1294, "Read from GPIO card failed, at (1)\n")));
        errno=errnoSave;
        perror(errBuf);
        fprintf(stderr, (nl_msg(1295, "read(%d,localBuffer,%d) returned %d expected %d\n")),
		       cfd,length+2,alen,length+2);
        return(FAIL);
   }
#ifdef DDBDEBUG
         fprintf(stderr,"GPIO_RD: calling handshake\n");
#endif DDBDEBUG

   /*
    *  Wait for the target to return to command mode
    */
   result = handshakeEIR(cfd,TIMEOUTLIMIT);
#ifdef ERRORTEST
   if ((result == FAIL) || (ddbErrTest(10) == TRUE))
#else
   if (result == FAIL)
#endif ERRORTEST
   {
        if (verboseErrors == YES)
           fprintf(stderr, (nl_msg(1297, "handshakeEIR(%d,%d) failed, at (3)\n")),
                                                    cfd,TIMEOUTLIMIT);
	return(FAIL);
   }

   /*  This should be impossible  */
   if (alen > length)
      alen = length;

   /* Now move the data into the caller's buffer  */
   from = (unsigned int *)localBuffer;
   to   = (unsigned int *)buffer;
   i = length / 4;

   while (i--)
      *to++ = *from++;

#ifdef DDBDEBUG
   fprintf(stderr,"GPIO_RD: returning\n");
#endif DDBDEBUG

   return (alen);
}

/*------------------------------------------------------------------------
 * gpio_wr - This process controls writing to the target.
 *
 *   1.  Send the WRITE command packet to the target to tell it we 
 *       want to send data to it.
 *
 *   2.  Wait for the target to configure to receive the data
 *       (handshakeEIR)
 *
 *   3.  Write the data
 *
 *   4.  If stoprunning == NO then
 *          HandshakeEIR (wait for the target to configure for a new
 *          command packet).
 *       else
 *          continue  (if we're processing the XDB quit command
 *             we must leave EIR high or the next invocation of
 *             ddb will hang waiting for an EIR which will never
 *             come.)
 *
 * CALL SEQUENCE: gpio_wr(file,buffer,length,address,mode,stoprunning)
 *       where: file    = comm link file descriptor
 *              length  = number bytes user wants transfered
 *              buffer  = length bytes starting at address in target
 *              address = addr in target of where to start writing
 *              mode    = WRITE | INTRUPT
 *              stoprunning= flag indicating if last handshakeEIR
 *                            is to occur.
 *
 * RETURN VALUES: return(results/FAIL)
 *       where: results = number of bytes actually transfered
 *
 * GLOBALS: errno (errno.h), errnoSave, errBuf
 *
 * gpio_wr is called from:  GpioWrite
 *                          spec_wr
 *
 * gpio_wr calls: handshakeEIR, send_command
 *
 * system routine(s) used: fprintf, nl_msg, perror, sprintf, write
 *----------------------------------------------------------------------*/
static int gpio_wr (cfd, buffer, length, address,mode,stoprunning)
int    cfd;             /* file descripter */
char   *buffer;         /* buffer address */
long   length;          /* number of bytes to read */
unsigned int address;   /* address to read or write from */
int    mode;            /* WRITE | INTRUPT */
int    stoprunning;     /* to handshake or not to handshake */
{
   register unsigned int *from, *to;
   int   alen, result, action;
   register int i;
   int errorflag;

#ifdef DDBDEBUG
   fprintf(stderr,"GPIO_WR: calling send_command\n");
#endif DDBDEBUG

   /*  Send the command packet  */
   result = send_command(cfd,address,length,mode);
#ifdef ERRORTEST
   if ((result == FAIL) || (ddbErrTest(12) == TRUE))
#else
   if (result == FAIL)
#endif ERRORTEST
   {
      if (verboseErrors == YES)
         fprintf(stderr, (nl_msg(1300, "send_command(%d,%x,%d,%d) failed, at (3)\n")),
                                              cfd,address,length,WRITE);
      return(FAIL);
   }

#ifdef DDBDEBUG
   fprintf(stderr,"GPIO_WR: calling handshake\n");
#endif DDBDEBUG

/*
 *  After the target rdb code has configured the mid-bus parallel
 *  card for input it will raise EIR.
 *
 *  Soo, lets wait.  Given the speed of an 840 it will probably
 *  be ready before we even get here.
 */
   result = handshakeEIR(cfd,TIMEOUTLIMIT);
#ifdef ERRORTEST
   if ((result == FAIL) || (ddbErrTest(13) == TRUE))
#else
   if (result == FAIL)
#endif ERRORTEST
   {
        if (verboseErrors == YES)
           fprintf(stderr, (nl_msg(1302, "handshakeEIR(%d,%d) failed, at (4)\n")),
                                                    cfd,TIMEOUTLIMIT);
	return(FAIL);
   }

#ifdef DDBDEBUG
   fprintf(stderr,"GPIO_WR: doing write\n");
#endif DDBDEBUG

/*
 *  ok, now write the data
 */
repeatwrite0:
   alen = write(cfd, buffer, length);
#ifdef ERRORTEST
   if ((alen != length) || (ddbErrTest(14) == TRUE))
#else
   if (alen != length)
#endif ERRORTEST
   {
      errnoSave=errno;
      /*
       * This kludge is because of a bug in the 300 kernel.
       * The kernel is counting the number and size of transfers
       * to the file.  This is ok, for real disc file and a bug
       * for device files.  We must close the file and the reopen
       * it to reset the counter.
       */
      errorflag=FALSE;
      if ((errno=22) && errorflag)
      {
	 errorflag=TRUE;
         if (get_comm_link(&cfd,'\0') == FAIL) 
         {
           if (verboseErrors == YES)
              fprintf(stderr, (nl_msg(1093, "get_comm_link(&cfd,NULL) failed\n")));
           return(FAIL);
         }
         goto repeatwrite0;
      }
      sprintf(errBuf,"%s",myname);
      fprintf(stderr, (nl_msg(1305, "write to gpio card failed, at (2)\n")));
      errno=errnoSave;
      perror(errBuf);
      fprintf(stderr, (nl_msg(1306, "write(%d,buffer,%d) wrote %d bytes, at (1)\n")),cfd,length,alen);
      return(FAIL);
   }

#ifdef DDBDEBUG
   fprintf(stderr,"GPIO_WR: calling handshake\n");
#endif DDBDEBUG

   /*
    *  If the user has requested ddb terminate and leave the target
    *  running, we do not want to "handshake" the EIR line.  If we
    *  do then there will be no EIR signal when we start up later.
    *
    *  EIR here indicates that the target has returned to command
    *  mode.  Once it's told us it will not tell us again (usually).
    */
   if (stoprunning == NO)
   {
      result = handshakeEIR(cfd,TIMEOUTLIMIT);
#ifdef ERRORTEST
      if ((result == FAIL) || (ddbErrTest(15) == TRUE))
#else
      if (result == FAIL)
#endif ERRORTEST
      {
           if (verboseErrors == YES)
              fprintf(stderr, (nl_msg(1308, "handshakeEIR(%d,%d) failed, at (5)\n")),
                                                    cfd,TIMEOUTLIMIT);
	   return(FAIL);
      }
   }

   if (alen > length)
      alen = length;

#ifdef DDBDEBUG
   fprintf(stderr,"GPIO_WR: returning\n");
#endif DDBDEBUG

   return alen;
}

/*------------------------------------------------------------------------
 * spec_wr
 *
 * This routine is called from the GpioIoctl routine when an AWRITE or
 * INTERRUPT command is encountered.
 *
 * CALL SEQUENCE: spec_wr(file,buf,addr,len,mode,stoprunning)
 *       where: file = comm link file descriptor
 *              buf  = data buffer to write to target
 *              addr = address in target of where to write buf
 *              len  = number of bytes to transfer 
 *              mode = ioctl command (INTERRUPT or AWRITE)
 *              stoprunning = xdb quit processing flag
 *
 * RETURN VALUES: return(SUCCESS/FAIL)
 *
 * GLOBALS: verboseErrors
 *
 * spec_wr is called from: gpioioctl
 *
 * spec_wr calls:  gpio_wr, handshakeEIR
 *
 * system routine(s) used: fprintf, nl_msg
 *----------------------------------------------------------------------*/

static int spec_wr(cfd, buf, offset, len, mode, stoprunning)
int  *cfd;             /* file descriptor */
char *buf;             /* addr of data buffer */
unsigned int offset;   /* target memory address for transfer */
int len;               /* number of bytes to be transfered    */
int mode;              /* AWRITE or INTERRUPT */
int stoprunning;       /* XDB quit processing flag */
{
   int rtrn;
   int result;

#ifdef DDBDEBUG
   fprintf(stderr,"SPEC_WR: calling gpio_wr w/WRITE\n");
#endif DDBDEBUG

/* 
 *  If this is an AWRITE, we are writing into the 'continue'
 *     flag.  This will cause a while loop in the target's rdb code 
 *     to terminate.  Control will then be returned to the 
 *     interrupted procedure.
 *
 *  If this is an INTERRUPT, we are writing the reason (SPECSTOP)
 *     for the next interrupt into ADR_INTP_REASON.
 */

   result = gpio_wr (cfd, buf, len, offset, WRITE, stoprunning);
#ifdef ERRORTEST
   if ((result == FAIL) || (ddbErrTest(17) == TRUE))
#else
   if (result == FAIL)
#endif ERRORTEST
   {
      if (verboseErrors == YES)
         fprintf(stderr, (nl_msg(1311, "gpio_wr(%d,buf,%d,%x,AWRITE,%d) failed, at (4)\n")),
						  cfd,len,offset,stoprunning);
      return(FAIL);
   }

   /*
    *  If this is an interrupt command cause the target to
    *    interrupt.  I suspect that we could use any command
    *    and this would work.  However, I know this works.
    *    Don't fix it if it isn't broken.
    */
   if (mode == INTERRUPT)
   {

#ifdef DDBDEBUG
      fprintf(stderr,"SPEC_WR: calling gpio_wr w/INTRUPT\n");
#endif DDBDEBUG

      result = gpio_wr (cfd, 0, 0, 0, INTRUPT, stoprunning);
#ifdef ERRORTEST
      if ((result == FAIL) || (ddbErrTest(18) == TRUE))
#else
      if (result == FAIL)
#endif ERRORTEST
      {
         if (verboseErrors == YES)
            fprintf(stderr, (nl_msg(1313, "gpio_wr(%d,0,0,0,INTRUPT,%d) failed, at (5)\n")),
							cfd,stoprunning);
         return(FAIL);
      }
   }

/*
 *  This is where things get confusing.
 *      If we are trying to INTERRUPT the target, the handshake
 *         in gpio_wr put us back into command mode in the target.
 *         Don't handshake again EIR has been cleared and
 *         wait_for_bits_set will hang.
 *
 *      If the user has requested ddb quit and leave the target
 *         running, don't handshake here because the target is 
 *         now in command mode.  A handshake here would cause
 *         the target to clear EIR and we would not be able
 *         to restart ddb.  syncWithTarget would hang in
 *         wait_for_bits_set waiting for EIR.
 */

   if ((stoprunning == NO) && (mode != INTERRUPT))
   {

#ifdef DDBDEBUG
      fprintf(stderr,"SPEC_WR: handshakeEIR\n");
#endif DDBDEBUG

      result = handshakeEIR (cfd, NEVERTIMEOUT);
#ifdef ERRORTEST
      if ((result == FAIL) || (ddbErrTest(19) == TRUE))
#else
      if (result == FAIL)
#endif ERRORTEST
      {
         if (verboseErrors == YES)
            fprintf(stderr, (nl_msg(1315, "handshakeEIR(%d,%d) failed, at (6)\n")),cfd,NEVERTIMEOUT);
         return(FAIL);
      }
   }

#ifdef DDBDEBUG
   fprintf(stderr,"SPEC_WR: returning\n");
#endif DDBDEBUG

   return (SUCCESS);
}

/*##########################################################################*/

/*------------------------------------------------------------------------
 *  ackEIR - This process acknowledges recognition of the  EIR
 *           signal from the target.
 *
 *  This is accomplished by raising and then lowering the control
 *  line on the 300's GPIO card which is connected to the tagret
 *  system's EIR line. (The EIR line on the mid-bus parallel card is
 *  edge triggered.)
 *
 *  We then wait for the target system to `drop' our EIR before we
 *  continue.
 *
 *  CALL SEQUENCE: ackEIR(cfd)
 *        where: cfd = comm link file descriptor
 *
 *  RETURN VALUES: return(SUCCES/FAIL)
 *       FAIL = an error occurred when we requested status from the 
 *              GPIO driver or the target didi not respond within
 *              the time out limit.
 *
 *  GLOBALS: errno (errno.h), errnoSave, errBuf, vttimer,
 *           zerotimer, old_vttimer, timeout
 *
 *  ackEIR is called from  handshakeEIR
 *                         check_ready
 *
 *  ackEIR calls: no ddb routines
 *
 *  system routine(s) used: fprintf, ioctl, nl_msg, perror, 
 *                          setitimer, sprintf
 *----------------------------------------------------------------------*/
static int ackEIR (cfd)
int *cfd;
{
   unsigned short   eir_reason;
   int   status;
   struct ioctl_type   args;

#ifdef DDBDEBUG
   fprintf(stderr,"ACKeir: entered\n");
#endif DDBDEBUG

   /*   Lets get the status of the target's control lines */
   args.type = HPIB_BUS_STATUS;
   status = ioctl (cfd,HPIB_STATUS,&args);
#ifdef ERRORTEST
   if ((status == FAIL) || (ddbErrTest(20) == TRUE))
#else
   if (status == FAIL)
#endif ERRORTEST
   {
      errnoSave=errno;
      sprintf(errBuf,"%s",myname);
      fprintf(stderr, (nl_msg(1319, "Unable to get GPIO status, at (5)\n")));
      errno=errnoSave;
      perror(errBuf);
      fprintf(stderr, "ioctl(%d,%d,&args)\n",cfd,HPIB_STATUS);
      return(FAIL);
   }
/*
 * isolate bits 28 and 31  (clear bits 29,30)
 */
   eir_reason = args.data[0] & (GP_PSTS | STI1);

/*
 * Reset the EIR.  We must request Indigo to do this and
 * then wait in a tight loop for it to be done.
 *
 * Send an Eir to Indigo */

   args.type = HPIB_SRQ;
   args.data[0] = GP_CTL0 | GP_CTL1;
   status = ioctl (cfd,HPIB_CONTROL,&args);
#ifdef ERRORTEST
   if ((status == FAIL) || (ddbErrTest(21) == TRUE))
#else
   if (status == FAIL)
#endif ERRORTEST
   {
      errnoSave=errno;
      sprintf(errBuf,"%s",myname);
      fprintf(stderr, (nl_msg(1322, "Unable to set GPIO control lines, at (1)\n")));
      errno=errnoSave;
      perror(errBuf);
      fprintf(stderr, "ioctl(%d,%d,&args)\n",cfd,HPIB_CONTROL);
      return(FAIL);
   }

/*
 *  The transition on the EIR (our ctl0) will cause the EIR
 *  interrupt in the target.
 */
   args.type = HPIB_SRQ;
   args.data[0] = GP_CTL1;
   status = ioctl (cfd,HPIB_CONTROL,&args);
#ifdef ERRORTEST
   if ((status == FAIL) || (ddbErrTest(22) == TRUE))
#else
   if (status == FAIL)
#endif ERRORTEST
   {
      errnoSave=errno;
      sprintf(errBuf,"%s",myname);
      fprintf(stderr, (nl_msg(1325, "Unable to set GPIO control lines, at (2)\n")));
      errno=errnoSave;
      perror(errBuf);
      fprintf(stderr, "ioctl(%d,%d,&args)\n",cfd,HPIB_CONTROL);
      return(FAIL);
   }

/*  Now wait for our EIR line to go low */

   vttimer.it_value.tv_sec      = TIMEOUTLIMIT;
   vttimer.it_value.tv_usec     = 0;
   vttimer.it_interval.tv_sec   = 0;
   vttimer.it_interval.tv_usec  = 0;
   timeout=0;
   if (setitimer(ITIMER_REAL, &vttimer, &old_vttimer) == -1)
   {
      perror(myname);
      fprintf(stderr,(nl_msg(1131,"Unable to set interval timer.  Timeouts not active.\n")));
   }

   do 
   {
      args.type = HPIB_BUS_STATUS;
      status = ioctl (cfd,HPIB_STATUS,&args);
#ifdef ERRORTEST
      if ((status == FAIL) || (ddbErrTest(24) == TRUE))
#else
      if (status == FAIL)
#endif ERRORTEST
      {
         errnoSave=errno;
         sprintf(errBuf,"%s",myname);
         fprintf(stderr, (nl_msg(1329, "Unable to get GPIO status, at (6)\n")));
         errno=errnoSave;
         perror(errBuf);
         fprintf(stderr, "ioctl(%d,%d,&args)\n",cfd,HPIB_STATUS);
         return(FAIL);
      }
   /*
    *  isolate bits 28,29, and 31.  clear bit 30
    *  wait for bits 28 and 31 to match original value and
    *      for bit 29 to go set (which is eir clear)
    */
   } while (!timeout && ((args.data[0] & (GP_PSTS | STI1 | GP_INT_EIR)) ==
            (eir_reason | GP_INT_EIR)));
   
#ifdef DDBDEBUG0
   if (getitimer(ITIMER_REAL,&get_vttimer) == -1)
   {
      perror(myname);
      fprintf(stderr,"getitimer failed, at(1)\n");
      return(FAIL);
   }
   fprintf(stderr," interval left  sec: %d\n",get_vttimer.it_value.tv_sec);
#endif DDBDEBUG0
   /*
    *  Turn off the timer.
    */
   if (setitimer(ITIMER_REAL,&zerotimer,&old_vttimer) == -1)
   {
       perror(myname);
       fprintf(stderr,(nl_msg(1133,"Unable to reset interval timer\n")));
       return(FAIL);
   }

   if (timeout)
   {
      fprintf(stderr, (nl_msg(1327, "Time out waiting for EIR to clear\n")));
      return(FAIL);
   }

   /* Deassert the EIR handshake line */
   args.type = HPIB_SRQ;
   args.data[0] = 0;
   status = ioctl (cfd,HPIB_CONTROL,&args);
#ifdef ERRORTEST
   if ((status == FAIL) || (ddbErrTest(25) == TRUE))
#else
   if (status == FAIL)
#endif ERRORTEST
   {
      errnoSave=errno;
      sprintf(errBuf,"%s",myname);
      fprintf(stderr, (nl_msg(1332, "Unable to set GPIO control lines, at (3)\n")));
      errno=errnoSave;
      perror(errBuf);
      fprintf(stderr, "ioctl(%d,%d,&args)\n",cfd,HPIB_CONTROL);
      return(FAIL);
   }

#ifdef DDBDEBUG
   fprintf(stderr,"ACKeir: returning\n");
#endif DDBDEBUG

   return(SUCCESS);
}

/*
 *-------------------------------------------------------------------------
 *  check_ready - This process verifies that the target system is ready
 *  to receive commands.  
 *
 *  Using the current parallel card handshake protocol this means:
 *    1. check that our EIR line is set.  (The target rdb code sets our EIR
 *       line after it has configured its DMA to read.)
 *    2. Then acknowledge the EIR.
 *
 * call sequence: check_ready(cfd)
 *       where: cfd = comm link file descriptor
 *
 * return values: return(SUCCESS/FAIL)
 *
 * globals: errno (errno.h), errnoSave, errBuf
 *
 * check_ready is called from: get_comm_link (ddbComm.c)
 *
 * check_ready calls: ackEIR
 *
 * system routine(s) used: fprintf, ioctl, nl_msg, perror, sprintf
 *
 *-------------------------------------------------------------------------
 */
int check_ready(cfd)
int *cfd;
{
   struct ioctl_type   args;
   register int   status;
   int result;


   /* If indigo has EIR set when we come up, tell him to clear EIR */
   args.type = HPIB_BUS_STATUS;
   status = ioctl (cfd,HPIB_STATUS,&args);
#ifdef ERRORTEST
   if ((status == FAIL) || (ddbErrTest(26) == TRUE))
#else
   if (status == FAIL) 
#endif ERRORTEST
   {
      errnoSave=errno;
      sprintf(errBuf,"%s",myname);
      fprintf(stderr, (nl_msg(1336, "Unable to get GPIO status, at (4)\n")));
      errno=errnoSave;
      perror(errBuf);
      fprintf(stderr, "ioctl(%d,%d,&args)\n",cfd,HPIB_STATUS);
      return(FAIL);
   }
   /* Now wait for the EIR line to clear */
   if (args.data[0] & GP_INT_EIR)
   {
      result = ackEIR(cfd);
#ifdef ERRORTEST
      if ((result == FAIL) || (ddbErrTest(27) == TRUE))
#else
      if (result == FAIL)
#endif ERRORTEST
      {
         if (verboseErrors == YES)
           fprintf(stderr, (nl_msg(1338, "ackEIR(%d) failed, at (5)\n")),cfd);
         return(FAIL);
      }
   }
   return(SUCCESS);
}

/*--------------------------------------------------------------------------
 * handshakeEIR - This process controls waiting for and
 * acknowledging the EIR signal from the target system.  Usually this
 * is used to indicate the target system has configured its DMA to
 * read a command.  See the State Transition Diagrams.
 *
 *  It is passed the file descriptor for the communication link to
 *  be used and a time out limit.  There are two values for the time
 *  out limit:
 *
 *  NEVERTIMEOUT - this will cause ddb to wait forever for the
 *                 target to raise our EIR line.  This is used
 *                 when the target is executing and we are
 *                 waiting for a breakpoint to be encountered.
 *
 *  TIMEOUTLIMIT - If the target doesn't respond by raising our EIR
 *                 line before this value is reached, FAIL is returned.
 *
 *  First we wait for the target system to raise our EIR line.
 *  Then we toggle the target system's EIR line.  This tells the
 *  target to drop our EIR line.  When our line goes `low', the
 *  handshake is complete.
 *
 *  The mid-bus parallel card is edge triggered.  Which is why we
 *  toggle the target's EIR line.
 *
 * CALL SEQUENCE: handshakeEIR(cfd,timeoutlimit)
 *       where: cfd ........ = file descriptor for comm link
 *              timeoutlimit = one of the value described above
 *
 * RETURN VALUES: return(SUCCESS/FAIL)
 *
 * GLOBALS: NONE
 *
 * handshakeEIR is called from:   gpio_rd
 *                                gpio_wr
 *                                GpioIoctl
 *                                spec_wr
 *
 * handshakeEIR calls: ackEIR, wait_for_bits_set
 *
 * system routine(s) used: fprintf
 *--------------------------------------------------------------------------
 */
 handshakeEIR(cfd,timeoutlimit)
 int *cfd;
 int timeoutlimit;
 {
   int result;

#ifdef DDBDEBUG
   fprintf(stderr,"handshake: calling wait_for_bits_set.\n");
#endif DDBDEBUG

   /*  Go wait for the EIR line to go logically high  */
   result = wait_for_bits_set (cfd, GP_INT_EIR, timeoutlimit);
#ifdef ERRORTEST
   if ((result == FAIL) || (ddbErrTest(28) == TRUE))
#else
   if (result == FAIL)
#endif ERRORTEST
   {
      if (verboseErrors == YES)
         fprintf(stderr, (nl_msg(1340, "wait_for_bits_set(%d,%x,%d) failed, at (2)\n")),
		cfd,GP_INT_EIR,timeoutlimit);
      return(FAIL);
   }
   
#ifdef DDBDEBUG
   fprintf(stderr,"handshake: calling ackEIR.\n");
#endif DDBDEBUG

/*  OK, we've seen EIR.  Now tell the target to drop it.  */
#ifdef ERRORTEST
   if ((ackEIR (cfd) == FAIL) || (ddbErrTest(29) == TRUE))
#else
   if (ackEIR (cfd) == FAIL)
#endif ERRORTEST
   {
      if (verboseErrors == YES)
         fprintf(stderr, (nl_msg(1342, "ackEIR(%d) failed, at (2)\n")),cfd);
      return(FAIL);
   }
#ifdef DDBDEBUG
   fprintf(stderr,"handshake: exiting.\n");
#endif DDBDEBUG

   return(SUCCESS);
}

/*------------------------------------------------------------------------
 *  init_gpio - This process sends the following configuration
 *  information to the ddb GPIO driver:
 *   1. set the card for a channel width of 16 bits
 *   2. set the card speed to a value which will always use DMA
 *       (An IO_SPEED value of 8.)
 *
 *  CALL SEQUENCE: init_gpio(cfd)
 *        where: cfd = comm link file descriptor
 *
 *  RETURN VALUES: return(SUCCESS/FAIL)
 *
 *  GLOBALS: errno (errno.h), errnoSave, errBuf
 *
 *  init_gpio is called from:  get_comm_link  (commLo.c)
 *
 *  init_gpio calls: no ddb routines
 *
 *  system routine(s) used: fprintf, ioctl, nl_msg, perror, sprintf
 *------------------------------------------------------------------------
 */
init_gpio (cfd)
int   *cfd;
{
   struct ioctl_type   args;
   register int   status;

   /* Set channel width to 16 bits */
   args.type = IO_WIDTH;
   args.data[0] = 16;      
   status = ioctl(cfd,IO_CONTROL,&args);
#ifdef ERRORTEST
   if ((status == FAIL) || (ddbErrTest(30) == TRUE))
#else
   if (status == FAIL)
#endif ERRORTEST
   {
      errnoSave=errno;
      sprintf(errBuf,"%s",myname);
      fprintf(stderr, (nl_msg(1345, "Unable to set GPIO width, at (1)\n")));
      errno=errnoSave;
      perror(errBuf);
      fprintf(stderr, "ioctl(%d,%d,&args)\n",cfd,IO_CONTROL);
      return(FAIL);
   }

   /* Set the card speed to a vaule which will always use DMA */
   args.type = IO_SPEED;
   args.data[0] = 8;
   status = ioctl(cfd,IO_CONTROL,&args);
#ifdef ERRORTEST
   if ((status == FAIL) || (ddbErrTest(31) == TRUE))
#else
   if (status == FAIL)
#endif ERRORTEST
   {
      errnoSave=errno;
      sprintf(errBuf,"%s",myname);
      fprintf(stderr, (nl_msg(1348, "Unable to set GPIO speed, at (1)\n")));
      errno=errnoSave;
      perror(errBuf);
      fprintf(stderr, "ioctl(%d,%d,&args)\n",cfd,IO_CONTROL);
      return(FAIL);
   }

   return(SUCCESS);
}

/*
 *--------------------------------------------------------------------------
 *  send_command - This process formats the command packet and sends it to
 *  the target.  It is passed:
 *   1.  the file descriptor of the communication link to use.
 *   2.  the address in the target system at which the I/O will start
 *   3.  the number of bytes we want to transfer
 *   4.  the action to be performed - READ or WRITE
 *
 *  A check is made to ensure the target is in input mode.  We will wait
 *  for our STI0 line to go low.  This line is connected to a line on the
 *  target system's mid-bus card which indicates which direction (read into
 *  or write out of the target) the card is configured for.
 *
 * CALL SEQUENCE:  send_command(cfd,addr,len,action)
 *       where:  cfd    = comm link file descriptor
 *               addr   = target address for start of I/O
 *               len    = number of bytes to transfer
 *               action = READ or WRITE
 *
 * RETURN VALUES: return(SUCCESS/FAIL)
 *
 * GLOBALS: errno (errno.h), errnoSave, errBUf
 *
 *  send_command is called from:   gpio_rd
 *                                 gpio_wr
 *                                 GpioIoctl
 *
 *  send_command calls: wait_for_bits_clear
 *
 *  system routine(s) used: fprintf, nl_msg, perror, sprintf, write
 *--------------------------------------------------------------------------
 */
 send_command(cfd,address,len,action)
 int *cfd;
 int address;
 int len;
 int action;
 {
   int result;
   int errorflag;
   struct cmdRec {
      unsigned int command, address, length;
   } cmd;

#ifdef DDBDEBUG
   switch (action)
   {
   case READ:
   case WRITE:
   case INTRUPT:
   case BOOTINDIGO:
       break;
   default:
       fprintf(stderr,"Unknown action %d in send_command\n",action);
       return(FAIL);
   }
   fprintf(stderr,"SEND_command: call wait_for_bits_clear\n");
#endif DDBDEBUG

/*
 * wait for the target to issue the read.  Wait for its
 * I/O line to go to input.
 */
   result = wait_for_bits_clear(cfd, STI0, TIMEOUTLIMIT);
#ifdef ERRORTEST
   if ((result == FAIL) || (ddbErrTest(32) == TRUE))
#else
   if (result == FAIL)
#endif ERRORTEST
   {
      if (verboseErrors == YES)
         fprintf(stderr, (nl_msg(1352, "wait_for_bits_clear(%d,%x,%d) failed, at (1)\n")),
                                         cfd,STI0,TIMEOUTLIMIT);
      return(FAIL);
   }
   
#ifdef DDBDEBUG
   fprintf(stderr,"SEND_command: writing command packet\n");
#endif DDBDEBUG

/*
 *  Now build the command packet and write it to the target
 */
   cmd.command = action;
   cmd.address = address;
   cmd.length  = len;
repeatwrite1:
   result = write (cfd,&cmd,sizeof(cmd)); 
#ifdef ERRORTEST
   if ((result != sizeof(cmd)) || (ddbErrTest(33) == TRUE))
#else
   if (result != sizeof(cmd))
#endif ERRORTEST
   {
      errnoSave=errno;
      /*
       * This kludge is because of a bug in the 300 kernel.
       * The kernel is counting the number and size of transfers
       * to the file.  This is ok, for real disc file and a bug
       * for device files.  We must close the file and the reopen
       * it to reset the counter.
       */
      errorflag=FALSE;
      if ((errno=22) && errorflag)
      {
	 errorflag=TRUE;
         if (get_comm_link(&cfd,'\0') == FAIL) 
         {
           if (verboseErrors == YES)
              fprintf(stderr, (nl_msg(1093, "get_comm_link(&cfd,NULL) failed\n")));
           return(FAIL);
         }
         goto repeatwrite1;
      }
      sprintf(errBuf,"%s",myname);
      fprintf(stderr, (nl_msg(1355, "write(%d,&cmd,%d) wrote %d bytes, at (2)\n")),cfd,sizeof(cmd),result);
      errno=errnoSave;
      perror(errBuf);
      fprintf(stderr, (nl_msg(1356, "command = %d, address = %x, len = %d\n")),
                           cmd.command,cmd.address,cmd.length);
      return(FAIL);
   }

#ifdef DDBDEBUG
   fprintf(stderr,"SEND_command: returning\n");
#endif DDBDEBUG

   return(SUCCESS);
}

/*
 *-------------------------------------------------------------
 *
 *  timeralarm - This procedure "handles" the timer SIGVTALRM
 *  signal.  "Handling" means to set the global variable
 *  timeout=1;
 *
 *  CALL SEQUENCE: called by OS on a timer signal
 *
 *  RETURN VALUES: none
 *
 *  GLOBALS: timeout
 *
 *  timeralarm calls: no ddb procedures
 *
 *  system routine(s) used: none
 *-------------------------------------------------------------
 */

timeralarm ()
{
    timeout=1;
    return(SUCCESS);
}

/*------------------------------------------------------------------------
 * wait_for_bits_clear - This process waits for a caller specified bit
 * pattern to occur on the GPIO lines or, if we are waiting forever, for an
 * interrupt signal to occur (hotCtrlC = YES).
 *
 * If the timeoutlimit is exceeded before all the indicated bits are clear,
 * an error is reported and FAIL is returned.
 *
 * There are two values for timeoutlimit:
 *    TIMEOUTLIMIT = currently 180 seconds
 *    NEVERTIMEOUT = 0
 *
 * CALL SEQUENCE: wait_for_bits_clear(cfd,bits,timeoutlimit)
 *       where: cfd ........ = comm link file descriptor
 *              bits ....... = bit pattern to wait for
 *              timeoutlimit = how long to wait (see above)
 *
 * return values: return (SUCCESS/FAIL)
 *
 * globals: hitCtrlC, errno (errno.h), errnoSave, errBuf
 *          vttimer, zerotimer, old_vttimer, timeout
 *
 *  wait_for_bits_clear is called from:  send_command
 *
 *  wait_for_bits_clear calls:  no ddb routines
 *
 *  system routine(s) used: fprintf, ioctl, nl_msg, perror, 
 *                          setitimer, sigblock, sigsetmask, sprintf
 *----------------------------------------------------------------------*/
static int wait_for_bits_clear (cfd, bits, timeoutlimit)
int   *cfd;
unsigned   bits;
int   timeoutlimit;
{
   int   status;
   struct ioctl_type   args;

#ifdef DDBDEBUG
   int debugcnt =0;

   fprintf(stderr,"wait_for_bits_CLEAR: entered.  timeoutlimit =%d\n",
	  timeoutlimit);
#endif DDBDEBUG

   /*
    *  If we are going to "wait forever", turn on delivery of
    *  the interrupt signal.  We can only allow user interrupts
    *  to be delivered when we are processing a run command.
    *  If we let them in at any other time, they will "kill"
    *  an I/O request and somebody upstream will die.  ddb can't
    *  handle a read or a write being interrupted.
    */
   if (timeoutlimit == NEVERTIMEOUT)
      sigsetmask (0);      /* enable control C interrupt to trap */
   else
   {
      vttimer.it_value.tv_sec      = TIMEOUTLIMIT;
      vttimer.it_value.tv_usec     = 0;
      vttimer.it_interval.tv_sec   = 0;
      vttimer.it_interval.tv_usec  = 0;
      timeout=0;
      if (setitimer(ITIMER_REAL, &vttimer, &old_vttimer) == -1)
      {
        perror(myname);
	fprintf(stderr,(nl_msg(1131,"Unable to set interval timer.  Timeouts not active.\n")));
      }
   }

   args.type = HPIB_BUS_STATUS;
   do 
   {
      status = ioctl (cfd,HPIB_STATUS,&args);
#ifdef ERRORTEST
      if ((status == -1) || (ddbErrTest(34) == TRUE))
#else
      if (status == -1) 
#endif ERRORTEST
      {
         errnoSave=errno;
         sprintf(errBuf,"%s",myname);
         fprintf(stderr, (nl_msg(1360, "Unable to get GPIO status, at (3)\n")));
         errno=errnoSave;
         perror(errBuf);
         fprintf(stderr, "ioctl(%d,%d,&args)\n",cfd,HPIB_STATUS);
         return(FAIL);
      }

      status = args.data[0];


#ifdef DDBDEBUG
      if (++debugcnt >= 21)
      {
	 debugcnt = 0;
	 fprintf(stderr,"wait_for_bits_CLEAR: hitCtrlC = %d\n",hitCtrlC);
      }
#endif DDBDEBUG

   /*
    *  See the comment in wait_for_bits_set about hitCtrlC
    */
   } while ((status & bits) && !hitCtrlC && !timeout);

#ifdef DDBDEBUG0
   if (getitimer(ITIMER_REAL,&get_vttimer) == -1)
   {
      perror(myname);
      fprintf(stderr,"getitimer failed, at(1)\n");
      return(FAIL);
   }
   fprintf(stderr," interval left  sec: %d\n",get_vttimer.it_value.tv_sec);
#endif DDBDEBUG0
   /*
   /*
    *  Turn off the timer.
    */
   if (setitimer(ITIMER_REAL,&zerotimer,&old_vttimer) == -1)
   {
       perror(myname);
       fprintf(stderr,(nl_msg(1133,"Unable to reset interval timer\n")));
       return(FAIL);
   }
   /*
    *  If we "waited forever", we turned on delivery of
    *  the interrupt signal.  Now we need to block its
    *  delivery again.
    */
   if (timeoutlimit == NEVERTIMEOUT)
   {
      sigblock (SIGINTMASK);
      if (hitCtrlC)
      {
         hitCtrlC = FALSE;
      }
   }
    
   if (timeout)
   {
      fprintf(stderr, (nl_msg(1362, "Time out in wait_for_bits_clear\n")));
      fprintf(stderr, (nl_msg(1363, "Waiting for GPIO status of %x\n")),bits);
      return(FAIL);
   }
#ifdef DDBDEBUG
   fprintf(stderr,"wait_for_bits_CLEAR: exiting hitCtrlC=%d\n",hitCtrlC);
#endif DDBDEBUG

  return(SUCCESS);
}

/*------------------------------------------------------------------------
 * wait_for_bits_set - This process waits for a caller specified bit
 * pattern to occur on the GPIO lines or, if we are waiting forever, for an
 * interrupt signal to occur (hotCtrlC = YES).
 *
 * If the timeoutlimit is exceeded before all the indicated bits are set,
 * an error is reported and FAIL is returned.
 *
 * There are two values for timeoutlimit:
 *    TIMEOUTLIMIT = currently 180 seconds
 *    NEVERTIMEOUT = 0
 *
 * CALL SEQUENCE: wait_for_bits_set(cfd,bits,timeoutlimit)
 *       where: cfd ........ = comm link file descriptor
 *              bits ....... = bit pattern to wait for
 *              timeoutlimit = how long to wait (see above)
 *
 * return values: return (SUCCESS/FAIL)
 *
 * globals: hitCtrlC, errno (errno.h), errnoSave, errBuf
 *          vttimer, old_vttimer, zerotimer, timeout
 *
 * wait_for_bits_set is called from:  handshakeEIR
 *
 * wait_for_bits_set calls: no ddb procedures
 *
 * system routine(s) used: fprintf, ioctl, nl_msg, perror, 
 *                         setitimer, sigblock, sigsetmask, sprintf
 *----------------------------------------------------------------------*/
static int wait_for_bits_set (cfd, bits, timeoutlimit)
int   *cfd;
unsigned   bits;
int   timeoutlimit;
{
   int   status;
   struct ioctl_type   args;

#ifdef DDBDEBUG
   int debugcnt=0;

   fprintf(stderr,"wait_for_bits_SET: entered.  timeoutlimit =%d\n",
	  timeoutlimit);
#endif DDBDEBUG


   /*
    *  If we are going to "wait forever", turn on delivery of
    *  the interrupt signal.  We can only allow user interrupts
    *  to be delivered when we are processing a run command.
    *  If we let them in at any other time, they will "kill"
    *  an I/O request and somebody upstream will die.  ddb can't
    *  handle a read or a write being interrupted.
    */
   if (timeoutlimit == NEVERTIMEOUT)
      sigsetmask (0);      /* enable control C interrupt to trap */
   else
   {
      vttimer.it_value.tv_sec      = TIMEOUTLIMIT;
      vttimer.it_value.tv_usec     = 0;
      vttimer.it_interval.tv_sec   = 0;
      vttimer.it_interval.tv_usec  = 0;
      timeout=0;
      if (setitimer(ITIMER_REAL, &vttimer, &old_vttimer) == -1)
      {
        perror(myname);
        fprintf(stderr,(nl_msg(1131,"Unable to set interval timer.  Timeouts not active.\n")));
      }
   }

   args.type = HPIB_BUS_STATUS;
   do {
      status = ioctl (cfd,HPIB_STATUS,&args);

#ifdef ERRORTEST
   if ((status == -1) || (ddbErrTest(36) == TRUE))
#else
      if (status == -1) 
#endif ERRORTEST
      {
         errnoSave=errno;
         sprintf(errBuf,"%s",myname);
         fprintf(stderr, (nl_msg(1368, "Unable to get GPIO status, at (2)\n")));
         errno=errnoSave;
         perror(errBuf);
         fprintf(stderr, "ioctl(%d,%d,&args)\n",cfd,HPIB_STATUS);
         return(FAIL);
      }

      status = args.data[0];

   /*
    *  Interrupt processing:
    *   1.  We are looping waiting for the target to hit a
    *       break point.
    *   2.  An interrupt signal comes in
    *   3.  A stopSpec is issued which causes this routine to
    *       be reentered.
    *   4.  We wait for specStop to be processed.  When
    *       completed by the target system this routine will 
    *       return to the CtrlC trap handler.
    *   5.  When the trap handler exits we return to this
    *       while loop.  The variable `hitCtrlC' tells us to 
    *       exit the loop because the target is stopped.
    */
#ifdef DDBDEBUG
    if (++debugcnt >= 21)
    {
       debugcnt = 0;
       fprintf(stderr,"wait_for_bits_SET: hitCtrlC = %d\n",hitCtrlC);
    }
#endif DDBDEBUG

   } while (!(status & bits) && !hitCtrlC && !timeout);

#ifdef DDBDEBUG0
   if (getitimer(ITIMER_REAL,&get_vttimer) == -1)
   {
      perror(myname);
      fprintf(stderr,"getitimer failed, at(1)\n");
      return(FAIL);
   }
   fprintf(stderr," interval left  sec: %d\n",get_vttimer.it_value.tv_sec);
#endif DDBDEBUG0
   /*
   /*
    *  Turn off the timer.
    */
   if (setitimer(ITIMER_REAL,&zerotimer,&old_vttimer) == -1)
   {
       perror(myname);
       fprintf(stderr,(nl_msg(1133,"Unable to reset interval timer\n")));
       return(FAIL);
   }

   /*
    *  If we "waited forever", we turned on delivery of
    *  the interrupt signal.  Now we need to block its
    *  delivery again.
    */
   if (timeoutlimit == NEVERTIMEOUT)
   {
      sigblock (SIGINTMASK);
      if (hitCtrlC)
      {
         hitCtrlC = FALSE;
      }
   }

   if (timeout)
   {
      fprintf(stderr, (nl_msg(1370, "Time out in wait_for_bits_set\n")));
      fprintf(stderr, (nl_msg(1363, "Waiting for GPIO status of %x\n")),bits);
      return(FAIL);
   }

#ifdef DDBDEBUG
   fprintf(stderr,"wait_for_bits_SET: exiting hitCtrlC=%d\n",hitCtrlC);
#endif DDBDEBUG

   return(SUCCESS);
}

#ifdef ERRORTEST
/*--------------------------------------------------------------------------
 * NAME: ddbErrTest
 * 
 * DESC: This procedure is used in error return testing.  If
 *       the sequence number passed in the call equals the
 *       test number specified by the user, return TRUE.
 *       Otherwise return FALSE.
 *
 * CALL SEQUENCE: ddbErrTest(test#)
 *
 * RETURN VALUES: return(TRUE/FALSE)
 *
 * GLOBALS: ddbErrNumber
 *          <global 2> = <description>
 *
 * ddbErrTest calls:  no other procedures
 *
 * system routine(s) used: NONE
 *--------------------------------------------------------------------------
 */
ddbErrTest(test)
int test;
{
   if (test == ddbErrNumber)
      return(TRUE);
   else
      return(FALSE);
}
#endif ERRORTEST

#endif
