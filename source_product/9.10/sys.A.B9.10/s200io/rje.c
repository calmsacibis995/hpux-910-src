/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/rje.c,v $
 * $Revision: 1.5.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:17:20 $
 */

#include "../h/param.h"
#include "../h/buf.h"
#include "../h/user.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../wsio/intrpt.h"
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../h/systm.h"
#include "../h/rjedriver.h" 
#include "../h/rje_ioctl.h" 

                  /********************************/
                  /*          DEFINITIONS         */
                  /********************************/

/*
 * All objects within this source module are made up of the following
 * tags and modifiers.  If the definition is external (ie. in a .h file)
 * a reference to the proper source file is given.
 *
 * TAGS:
 * -----
 *
 * AEv             Asynchronous Event block
 * caddr_t         character address typedef               (sys/types.h)
 * Cond            interrupt Condition
 * Data            total Data on card
 * DD              Data Descriptor on PDI card
 * Dev             interrupting Device
 * dev_t           device number typedef                   (sys/types.h)
 * Int             Interrupt enable register
 * interrupt       interrupt struct                       (sys/intrpt.h)
 * isc_table       array of isc_table_type structs        (sys/hpibio.h)
 * isc_table_type  interface driver space struct          (sys/hpibio.h)
 * MINISC          macro constant                         (sys/hpibio.h)
 * m_busaddr       field def w/in minor no. macro      (sys/sysmacros.h)
 * m_selcode       field def w/in minor no. macro      (sys/sysmacros.h)
 * EXTERNALIAC     macro constant                         (sys/hpibio.h)
 * RJE_READ        rje_ioctl argument macro                (rje_ioctl.h)
 * RJE_WRITE       rje_ioctl argument macro                (rje_ioctl.h)
 * RJE_CONT_LINK   rje_ioctl argument macro                (rje_ioctl.h)
 * RJE_CONT_PDI    rje_ioctl argument macro                (rje_ioctl.h)
 * RJE_CONT_TRACE  rje_ioctl argument macro                (rje_ioctl.h)
 * RJE_REP_TRANS   rje_ioctl argument macro                (rje_ioctl.h)
 * RJE_REP_ERROR   rje_ioctl argument macro                (rje_ioctl.h)
 * RJE_SELF_TEST   rje_ioctl argument macro                (rje_ioctl.h)
 * RJ_data         RJE data structure per port         (sys/rjedriver.h)
 * RJ_stat         RJE status struct per SC            (sys/rjedriver.h)
 * RSB             Result Block
 * SC              Select Code
 * sleep           sleep -- wakeup pair macro                 (os/slp.c)
 * spl6            set priority level 6 macro                (ml/mchs.s)
 * splx            reset priority lvl macro, splx(spl6())    (ml/mchs.s)
 * STB             STatus Block
 * testr           testr(a,w), return 1 if a and a+w-1
 *                 are valid addr's, otherwise return 0.     (ml/mchs.s)
 * TId             integer rep of req code, f(req)=TId
 * uio             uio struct                                (sys/uio.h)
 * u               user structure                           (sys/user.h)
 * wakeup          sleep -- wakeup pair macro                 (os/slp.c)
 *
 * MODIFIERS:
 * ----------
 *
 * p       pointer
 * a       address
 * i       index into a linear array of objects
 * l       length
 * r       register value on pdi card
 * _type   "C" typedef or struct
 * t       temporary object, local significance only
 */

                  /********************************/
                  /*      MISC. CONSTANTS         */
                  /********************************/

#define MAX_RDWR_SIZE  768     /* Max read/write data allowable = 2   */
                               /* PDI buffers (3*256=768 bytes each). */
#define MAX_AEV_SIZE   3       /* Max bytes in any AEV block for RJE; */
#define TX_TYPE_READ   1       /* The the 3 transaction types used  by*/
#define TX_TYPE_WRITE  2       /* the card.                           */
#define TX_TYPE_IOCTL  3
#define RJE_PRIORITY   PZERO+1 /* Priority used by read/write/ioctl   */
                               /* when they sleep waiting wakeup from */
                               /* ISR (rjeintr). Must be >PZERO to be */
                               /* interruptable by SIGNAL's           */
#define PORT_MASK      0x20    /* PDI card port no. (=0 or 1) within  */
                               /* transaction. no's                   */
#define ERROR          -1      /* Error return value for all functions*/
#define SUCCESS         0      /* Normal return value for all func.'s */

                  /********************************/
                  /*   PDI HARDWARE DECLARATIONS  */
                  /********************************/
/*
 * Many of the constants defined here are for hardware registers and
 * their values.  Where possible the hardware register address on the
 * card has been grouped with its possible values.  A full table of the
 * hardware registers available on the card along with their addresses
 * is listed here for reference (this information is also available in
 * the DIO Mainframe/Backplane Interface document for the Alvin PDI card
 * from RND).
 *
 *   Card    Mainframe   Number
 *  Address   Address   of bytes  Description
 * --------------------------------------------------------------------
 *   8000H     0001H         1    Reset register
 *   8001H     0003H         1    Interrupt enabled register
 *   8002H     0005H         1    Semaphore register for shared mameory
 *   8003H     0007H         1    Modem control/status register
 *   A000H     4001H         1    INT-COND, card wrt intrpt mainframe
 *   A001H     4003H         1    COMMAND, mainframe wrt intrpt card
 *   A002H     4005H         1    Backplane error code
 *   A003H     4007H         1    Request block length
 *   A004H     4009H         2    Request block address
 *   A006H     400DH         3    Status block
 *   A009H     4013H         2    Total data area length
 *   A00BH     4017H         2    Data Descriptor address
 *   A00DH     401BH         1    Result block length
 *   A00EH     401DH         2    Result block address
 *   A010H     4021H         1    AEV block length
 *   A011H     4023H         2    AEV block address
 *   A013H     4027H         2    Abort transaction ID
 *   A015H     402BH         1    Fatal error code
 *   A016H     402DH         1    Self test error code
 *   A017H     402FH         1    Firmware ID (5 for 98641A)
 *   A018H     4031H         1    Interrupting port
 */

#define RESET_REG         0x0001  /* Hard reset/Firmware ID register  */
#define RESET_CARD_BIT    0x80    /* Reset the card                   */
#define FIRMWARE_ID       0x34    /* All ALVIN-based cards ID         */
#define FIRMWARE_ID_MASK  0x7F    /* 7 bit mask                       */
 
#define INT_REG           0x0003  /* interrupt/DMA register           */
#define INT_ENAB_BIT      0x80    /* "intrpt enabled" bit             */
#define INT_REQ_BIT       0x40    /* "intrpt being requested" bit     */

#define CARD_SEMA         0x0005
#define SEMA_ENAB_BIT     0x80

#define INT_COND          0x4001  /* Interrupt condition location.    */
#define ERROR_BIT         0x01    /* Error interrupt bit--all ALVIN's.*/
#define IC_XAC_STAT_AVAIL 0x1     /* These 4 are bit codes stored in  */
#define IC_AEV_AVAIL      0x2     /* INT_COND by PDI-ALVIN card only, */
#define IC_BKPLN_ERR      0x4     /* See DIO domumentation for defn.  */
#define IC_FATAL_ERR      0x8

#define COMMAND           0x4003 /* Driver writes 1 of these 7 values */
                                 /* to card as its next command. See  */
#define CONTINUE_XAC      1      /* DIO documentation for definition. */
#define XAC_COMPLETE      2
#define STAT_BLK_RCVD     3
#define NEW_REQ_AVAIL     4
#define AEV_RECVD         5
#define ABORT_XACTN       6
#define ERROR_RECVD       7

#define REQ_BLK_ADR      0x4009  /* Location of next request block    */

/*
 * Note:  that 0x4000D has 2 uses; one immediately after powerup, and
 *        one after:
 */

#define ERROR_CODE       0x400D  /* Error condition after hard reset- */
                                 /* all ALVIN--type cards.            */

#define STATUS_BLOCK     0x400D  /* Status of current transaction put */
                                 /* here by PDI.                      */
#define DATA_XFER_WAIT   1       /* See DIO documentation for defn.   */
#define END_XACTION      2
#define XACTION_ABORTED  3

#define DATA_AREA_LEN     0x4013  /* Data size in read/write xaction  */
#define DATA_DESCR_ADR    0x4017  /* Data addr in read/write xaction  */
#define TEST_CONNECTOR    0x401B  /* 0=None, 1=Card edge, 2=Cable end */
                                  /* 401B has TEST_CONNECTOR result   */
                                  /* only immediately after reset.    */
#define RSLT_STAT_BLK_LEN 0x401B  /* Result status block xaction ln.  */
#define RSLT_STAT_BLK_ADR 0x401D  /* Result status block addr (RSB)   */
#define AEV_BLK_LEN       0x4021  /* Async. EVent block size (AEV).   */
#define AEV_BLK_ADR       0x4023  /* AEV block addr.                  */
#define ABORT_TRANS_ID    0x4027  /* Xactn ID to be aborted.          */
#define FATAL_ERRCODE     0x402B  /* Fatal error code                 */
#define SELFTEST_ERR_CODE 0x402D  /* Error result of selftest         */
#define PORT_ID           0x4031  /* Addr of interrupting Port# (0,1) */

                  /********************************/
                  /*       EXPANDING MACRO'S      */
                  /********************************/

#define aDev pRJ_data->iocard               /* Macro for card address */

/*
 * RJE_DISABLE_INT(addr) and RJE_ENABLE_INT(addr) disables and enables interrupt
 * respectively of pdi card with base address 'addr.' 'addr' should be
 * of type 'char *'.
 */

#define RJE_ENABLE_INT(addr)  *(addr + INT_REG) = INT_ENAB_BIT;
#define RJE_DISABLE_INT(addr) *(addr + INT_REG) = 0x00;

/*
 * Computation of addresses & numerical values stored on the card
 */

#define twobytevalue(addr,base) (*((int)addr+base)+ *((int)addr+base+2)*256)
#define z80_to_68000(addr) (unsigned char *)(((int)twobytevalue(addr,aDev)-0xA000)*2 + 0x4001 + aDev)
#define z80(raw) (unsigned char *)(((int)raw-0xA000)*2 + 0x4001 + aDev)

/*
 * Computation of TId (transaction ID) given its request type, and its
 * inverse function (almost).
 */

#define rjcompute_TId(req) ((((unsigned char)(0xF & req)-1) %3) +1)
#define rjtrans_type(stb) (stb.transaction_id & 0x1F)
#define port(TId) (TId&PORT_MASK)>>5      /* Port ID from xaction no. */

/*
 * Verification of addresses.
 */

#define TEST1(loc) if(!testr(loc,1)) return(ERROR);
#define TEST2(loc) if((!testr(loc,1))||(!testr(loc+2,1))) return(ERROR);

                  /********************************/
                  /* GLOBAL VARIABLE DECLARATIONS */
                  /********************************/

struct RJ_stat_type mpSCRJ_stat[32];     /* RJ_stat_type/select code */
unsigned char       *rjwait_RQBA_NZ();
int                 gerror;              /* errno returned from lower*/
                                         /* levels to rd/wrt/ioctl   */

                  /********************************/
                  /********************************/
                  /*     ROUTINES INVOKED BY      */
                  /*       CARD INTERRUPTS        */
                  /********************************/
                  /********************************/

/*
 * TAGS:
 *   SC      = Select Code
 *   Dev     = interrupting Device
 *   Int     = Interrupt enable register
 *   Cond    = interrupt Condition
 *   AEv     = Asynchronous Event block
 *   DD      = Data Descriptor on PDI card
 *   RSB     = Result Block
 *   Data    = total DATA length on card
 *   TId     = maps request codes into Transaction Id's, f(req)=TId
 *   RJ_data = RJE data structure per port
 *   STB     = STatus Block
 *   RJ_stat = RJE status structure per select code
 *
 * MODIFIERS:
 *   p     = pointer
 *   a     = address
 *   i     = index into a linear array of objects
 *   l     = length
 *   r     = register value on pdi card
 *   _type = "C" typedef or struct
 *   t     = temporary object, local significance only
 */

/*
 * Called for all interrupts from I/O cards which were claimed at
 * powerup by kernel_initialize (in kernel.c) as being PDI cards (driver
 * number = 15).
 */

rjeintr(dev)
struct interrupt *dev;
{
unsigned int         rInt;               /* Interrupt enable register */
unsigned int         rCond;              /* Interrupt condition(s)    */
unsigned int         SC;                 /* Select Code               */
unsigned char        *pAEv;              /* ->asynchronous Event blk  */
unsigned char        *pDD;               /* ->Data Descriptor         */
unsigned char        *pDev;              /* ->Device                  */
unsigned char        *pRSB;              /* ->Result Block            */
short                lAEv;
short                i;
short                lData;
short                lRSB;
short                TId;
int                  RC;                 /* Result Code from func's   */
                                         /* non-cumulative, either =  */
                                         /* ERROR or SUCCESS.         */
int                  iRSB;
struct RJ_data_type  *pRJ_data;
struct RSB_type      tRSB;
struct STB_type      tSTB;

/*
 * Calcuate physical address of interrupting device from addr of inter-
 * rupt enable register specified in isrlink call in rje_make_entry.
 * Derive the select code from the physical address.  Card ports occur
 * at 64K add intervals on the bus at 600000 thru 7F0000 allowing for 32
 * external devices.  Select codes = (address-600000)/10000 = {0..31}.
 * Validate the physical address and select code derivation.
 *
 * NOTE: if this test should fail there is the possibility that the ker-
 * nal will hang.  Since the interrupt is not cleared on the card, if it
 * always interrupts with the same invalid address, the driver will fall
 * into a tight loop of interrupt-->uncleared-->interrupt ...
 */

RC = SUCCESS;
pDev = (unsigned char *) dev->regaddr-INT_REG;
SC   = ((int) pDev >> 16) - 0x60;
if (((int)pDev & 0xFFFF) ||        /* address not multiple of 0x10000 */
    (SC < MINISC)        ||        /* select code < 0                 */
    (SC > EXTERNALISC)) return(0); /* select code > 31                */

/*
 * Save interrupt information.
 */

rInt  = *(INT_REG + pDev);   /* Save value of interrupt reg */
RJE_DISABLE_INT(pDev);           /* Write interrupt reg to 0    */
rCond = *(INT_COND + pDev);  /* Save interrupt condition    */

/*
 * Begin error checking:
 *
 * Check whether this select code and port have been claimed by card.
 * An isc_table_type structure is created for the card and its pointer
 * stored in isc_table[select code] by hp-ux at power-up time.  The
 * RJ_stat_type structure in mpSCRJ_stat should have been allocated for
 * the non-trace port in make_rje_entry (called at powerup).  Therefore,
 * if the card was not recognized or we were unable to create a working
 * structure for it, return.
 *
 * NOTE:  Interrupts have been disabled above.  If this return is taken
 * the card will not be able to interrupt us again and the emulator will
 * hang.
 */

if ((isc_table[SC] == NULL) ||        /* Card not recognized          */
    (mpSCRJ_stat[SC].port0 == NULL))  /* RJ_stat_type not created     */
    return(0);

/*
 * pDev is derived from an address which we tell hp-ux to pass us when
 * we are interrupted in interrupt.regaddr (specified in isrlink() in
 * rje_make_entry().  mpSCRJ_stat[SC].port0->iocard is the card addr
 * derived at powerup time from the select code given us by hp-ux in
 * the isc_table_type structure passed to rje_make_entry by the kernel.
 * This is merely a consistancy test which should never fail, the only
 * reason it remains from the original code is that GCS couldn't figure
 * out why it's here in the first place, and therefore could not reason
 * why it should be removed.
 *
 * NOTE:  If this return is taken, even though interrupts are reenabled,
 * this and other pending interrupts (transaction available, backplane
 * error, fatal error, transaction aborted) are not acknowledged to the
 * card.  Therefore, if this was a valid interrupt that got garbled, it
 * will remain on the card, but will not interrupt again, and will not
 * allow any other interrupts of that type until it is acknowledged.
 * This may hang the emulator.  The symptom to look for is the card sit-
 * ting in a WACK or TTD loop.
 */

if(pDev != mpSCRJ_stat[SC].port0->iocard) {
    RJE_ENABLE_INT(pDev);
    return(0);
    }

/*
 * Retrieve RJ_data_type struct address from structure array.
 */

if (*(pDev + PORT_ID)) pRJ_data = mpSCRJ_stat[SC].port1;
else                   pRJ_data = mpSCRJ_stat[SC].port0;

/*
 * Check bit 6 of interrupt register to make sure card is interrupting,
 * if not ignore this interrupt but allow others. See the NOTE on the
 * previous error ck, this return suffers from the same possibilities:
 * It may hang, i.e. if a transaction interrupt is pending but bit 6 is
 * not set, the interrupt will not be acknowledged and no further trans-
 * action interrupts will be possible. The driver will hang on the next
 * read/write/control subfunction.
 */

if ((rInt & INT_REQ_BIT)==0) {
    internal_error(pRJ_data,0x01,4,(unsigned int)rInt,0);
    RJE_ENABLE_INT(pDev);
    return(0);
    }

/*
 * Since the card may have to wait for the mainframe to handle higher
 * level interrupts, several interrupt conditions may be pending before
 * rjeintr (RJE's ISR) is called, but a single read of the interrupt
 * condition register clears the interrupt line.  Therefore all 4 pos-
 * sible interrupt conditions must be handled per interrupt.  This is
 * sufficient since the card guarantees not to repeat any of the 4
 * interrupts until any earlier occurance of the same interrupt has been
 * acknowledged by the mainframe and rjeintr running as an ISR for the
 * driver cannot be interrupted by the card (interrupting at the same
 * interrupt level as rjeintr is running at -- will be pending).
 *
 * Additionally, the card has a priority on interrupt conditions as
 * follows which should be recognized by the mainframe:
 *     1.) Backplane and fatal errors.
 *     2.) Asynchronous events.
 *     3.) Transaction status.
 *
 * If there is a transaction status available among the interrupts,
 * store it's information (status block, data descriptor address and
 * length, and result block address and length).
 */

if(rCond&IC_XAC_STAT_AVAIL) {
    tSTB.transaction_status = (char) *(STATUS_BLOCK+aDev);
    tSTB.transaction_id     = (short)twobytevalue(STATUS_BLOCK+2,aDev);
    pDD    = z80_to_68000(DATA_DESCR_ADR);
    lData  = twobytevalue(DATA_AREA_LEN,aDev);
    pRSB   = z80_to_68000(RSLT_STAT_BLK_ADR);
    lRSB   = *(RSLT_STAT_BLK_LEN+aDev);
    }

/*
 * Handle interrupts by priority employed on card, if there has been
 * a backplane error consider it fatal (along with fatal error) and
 * do not service any other interrupt.  FATAL is set to indicate to
 * rjinitiator_stop that no further requests may be made on the card,
 * FATAL will be cleared on the next rje_open call.  A wakeup is done
 * on each port by rjrestart_initiator to give an error return on any
 * request currently pending on either port.
 */

if ((rCond & IC_FATAL_ERR) || (rCond & IC_BKPLN_ERR)) {
    internal_error(pRJ_data,0x04,1,(unsigned int)*(FATAL_ERRCODE+aDev),0);
    pRJ_data->driverstate = 0;
    mpSCRJ_stat[SC].port1->FATAL = 1; /* Indicate error on both ports */
    mpSCRJ_stat[SC].port0->FATAL = 1;
    rjreset(pRJ_data);   /* reset card to powerup state, link dropped */
    rjrestart_initiator(mpSCRJ_stat[SC].port1, ERROR); /* Wakeup both */
    rjrestart_initiator(mpSCRJ_stat[SC].port0, ERROR); /* ports       */
    }

/*
 * Handle asynchronous event.  We must handle the null asynchronous
 * event which the card uses to see if the host is still up before
 * starting each transmission (STX through ETX, not each transmission
 * block).  Here the driver merely stores the asynchronous event block
 * and acknowledge the event to the card, no interrupt is generated.
 */

else {
    if (rCond & IC_AEV_AVAIL) {
        lAEv = *(AEV_BLK_LEN + aDev);
        if ((lAEv < 2) || (lAEv > MAX_AEV_SIZE)) {
            internal_error(pRJ_data,0x05,1,(unsigned int)lAEv,0);
            RC = ERROR;
            }
        else {
            pAEv = z80_to_68000(AEV_BLK_ADR);
            if (testr(pAEv,3)==1) {
                pRJ_data->last_AEV.event_code = *pAEv;
                pRJ_data->last_AEV.port_id = *(pAEv+2);
                for (i=0; i<lAEv-2; i++)
                    if (testr(pAEv+4+i*2,1)==1)
                        pRJ_data->last_AEV.other_info[i] = *(pAEv+4+i*2);
                    else {
                        RC = ERROR;
                        break;
                        }
                if (RC==SUCCESS) RC = rjnew_command(AEV_RECVD,pRJ_data);
                }
            else RC = ERROR;
            }
        }
    if (RC==ERROR) rjreset(pRJ_data);

    RC = SUCCESS;
    if (rCond & IC_XAC_STAT_AVAIL)
        switch(tSTB.transaction_status)  {

        case DATA_XFER_WAIT:
            if (rjtrans_type(tSTB) == TX_TYPE_READ)
                RC = rjcard_data_move(tSTB.transaction_id,
                         pDD,lData,0,pRJ_data);
            else
                RC = rjcard_data_move(tSTB.transaction_id,
                         pDD,lData,1,pRJ_data);
            if (RC==SUCCESS) RC = rjnew_command(CONTINUE_XAC,pRJ_data);

            /*
             * If there was an error in moveing the data awake the pro-
             * cess waiting for an END_XACTION on this request.
             */

            if (RC==ERROR) {
                rjrestart_initiator(pRJ_data, ERROR);
                rjreset(pRJ_data);
                }
            break;

        case END_XACTION: do {

            /*
             * Index RSB by request_code.  Rjinitiator_stop puts
             * request_code in pRJ_data->semafore. Rjrestart_initiator
             * puts -1 in pRJ_data->semafore if there has been a previous
             * error on this command.
             */

            iRSB = (int)pRJ_data->semafore;
            if(iRSB < 0) iRSB = 0;
            
            /*
             *  Request code and value   rjcompute_TId value
             * ------------------------  -------------------
             * READ LINK DATA      0001           1
             * WRITE LINK DATA     0010           2
             * CONTROL LINK        0011           3
             * READ PDI            0100           1
             * WRITE PDI           0101           2
             * CONTROL PDI         0110           3
             * READ TRACE BLOCK    0111           1
             * UNSOLICITED EVENTS  1000           2
             */

            TId = tSTB.transaction_id & 0xF;
            if ((TId < 1) || (TId > 3)) {
                internal_error(pRJ_data,0x0B,1,(unsigned int)TId,0);
                RC = ERROR;
                break;
                }

            /*
             * Test address of Result Status Block (RSB) and copy
             * it into driver space.  Result Status Block is only
             * available when a transaction completes.
             *
             *    +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
             *    |         RESULT STATUS         |
             *    +---+---+---+---+---+---+---+---+
             *    |                     high byte |
             *    +--      TRANSFER LENGTH      --+
             *    |                      low byte |
             *    +---+---+---+---+---+---+---+---+
             *    |       ADDITIONAL STATUS       |
             *    +---+---+---+---+---+---+---+---+
             */
            if ((testr(pRSB,1)+testr(pRSB+2,3))!=2) {
                RC = ERROR;
                break;
                }
            else {
                pRJ_data->last_RSB[iRSB].status = *(pRSB);
                tRSB.status = pRJ_data->last_RSB[iRSB].status;
                pRJ_data->last_RSB[iRSB].transfer_length =
                    twobytevalue(2,pRSB);
                tRSB.transfer_length =
                    pRJ_data->last_RSB[iRSB].transfer_length;
                for (i=0; i<lRSB-3; i++)
                    if (testr(pRSB+6+i*2,1)!=1) {
                        RC = ERROR;
                        break;
                        }
                    else {
                        pRJ_data->last_RSB[iRSB].other_data[i] =
                            *(pRSB+6+i*2);
                        tRSB.other_data[i] =
                            pRJ_data->last_RSB[iRSB].other_data[i];
                        }
                if (RC==ERROR) break;
                }

            /*
             * If the transaction ended without error and it was a
             * read and there is data yet to read (pDD != * NULL),
             * transfer it from the card.
             */

            if (tRSB.status==(char)0)
                if ((rjtrans_type(tSTB)==TX_TYPE_READ) && (pDD!=NULL))
                    RC = rjcard_data_move(tSTB.transaction_id,
                          pDD,lData,0,pRJ_data);

            } while (FALSE);

            if (RC==SUCCESS) RC = rjnew_command(STAT_BLK_RCVD,pRJ_data);
            if (RC==ERROR) rjreset(pRJ_data);

            /*
             * GCS 3/27 -- driver not recognizing errors reported in re-
             * sult status block.
             */

            if (tRSB.status!=(char)0) RC = ERROR;
            rjrestart_initiator(pRJ_data, RC);
            break;

        /*
         * Transaction is either an abort transaction or not recognized
         * by the driver, treat as error by not acknowledging and waking
         * up any currently pending requests with error return to
         * rjinitiator_stop.  It is assumed that this is not a fatal 
         * error or it would be recognized as such by the card.
         */

        default:
            internal_error(pRJ_data,0x03,1,
                (unsigned int)(tSTB.transaction_status),0);
            rjrestart_initiator(mpSCRJ_stat[SC].port1, ERROR); /* Wakeup both */
            rjrestart_initiator(mpSCRJ_stat[SC].port0, ERROR); /* ports       */
            RC = ERROR;

        }  /* end of switch */
    if (RC==ERROR) pRJ_data->driverstate = 0;
    }  /* end of else */
RJE_ENABLE_INT(pDev);                /* Enable card to interrupt us again */
return(SUCCESS);
}

rjrestart_initiator(pRJ_data, error)
   struct RJ_data_type  *pRJ_data;
   int                  error;       /* ERROR propogate error to user */
{
int old_level;
if(error > 0) error = 0;

/***********************************************************************
*                                                                      *
* <<<<<<<<<<<<<<<<<<<<     START CRITICAL SECTION     >>>>>>>>>>>>>>>> *
*                                                                      *
***********************************************************************/

old_level = spl6();    /* (s)et(p)riority(l)evel=(6), only interrupts */
                       /* levels >= 6 can interrupt.                  */
pRJ_data->semafore = error;  /* See rjinitiator_stop for significance */
wakeup((caddr_t) &pRJ_data->semafore);
splx(old_level);       /* reset old priority level                    */

/***********************************************************************
*                                                                      *
* <<<<<<<<<<<<<<<<<<<<<     END CRITICAL SECTION     >>>>>>>>>>>>>>>>> *
*                                                                      *
***********************************************************************/

return(SUCCESS);
}

/*
 * This function utilizes the Transaction Data Descriptor data structure
 * found on the DIO card:
 *
 *    |------------- 1 byte --------------|
 *    +-----------------------------------+
 *    |       Number of Data Blocks       |
 *    +-----------------------------------+  --+--
 *    |  1st data block length  low  byte |    |
 *    +-----------------------------------+    |
 *    |  1st data block length  high byte |   one
 *    +-----------------------------------+ segment
 *    |  1st data block address low  byte |    |
 *    +-----------------------------------+    |
 *    |  1st data block address high byte |    |
 *    +-----------------------------------+  --+--
 *    |                                   |
 *    +-----------------------------------+
 *    | Last data block length  low  byte |
 *    +-----------------------------------+
 *    | Last data block length  high byte |
 *    +-----------------------------------+
 *    | Last data block address low  byte |
 *    +-----------------------------------+
 *    | Last data block address high byte |
 *    +-----------------------------------+
 *
 *    Where each data block length byte pair has the following syntax:
 *
 *    +---+---+---+---+---+---+---+---+
 *    |                 Data Block    |
 *    +---+---+---+---+   length      |
 *    | 0 | W | D | 0 |  (12 bits)    |
 *    +---+---+---+---+---+---+---+---+
 *
 *    W:  If set the data block is for a write transaction.  This bit is
 *        unecessary for DIO transactions, but is set or cleared by the
 *        midplane firmware for CIO interfaces.
 *
 *    D:  If set the data is stored backwards (high addr to low addr).
 */

 rjcard_data_move(TId, pDD, lData, flag, pRJ_data)
       short                TId;
       unsigned char        *pDD;
       short                lData;
       unsigned char        flag;                  /* 0 read; 1 write */
       struct RJ_data_type  *pRJ_data;

{
unsigned int  num_blocks, iblock, ibyte, next_seg_length;
int           direction;
unsigned char *next_seg_addr, *next_seg_addr_addr, *next_local_addr;

/*
 * Check that data will fit in pRJ_data->driv_databuf.
 */

if (lData > MAX_RDWR_SIZE) {
    internal_error(pRJ_data,0x0F,1,(unsigned int)lData,0);
    return(ERROR);
    }

if (testr(pDD,3)!=1) return(ERROR);  /* ck validity of addr  */

num_blocks         = *pDD;
next_seg_addr_addr = pDD+2;
next_local_addr    = pRJ_data->driv_databuf;

/*
 * This outer loop reads each transaction data descriptor segment and
 * calculates the length and address of the next block of data and the
 * direction in which it is stored.  Error checks are made on the data
 * block addresses validity and data block length byte pair syntax.
 */

for (iblock=0; iblock<num_blocks; iblock++) {
    next_seg_length = (unsigned int) twobytevalue(0,next_seg_addr_addr);
    next_seg_addr = z80_to_68000((next_seg_addr_addr - aDev)+4);
    if (testr(next_seg_addr,1)!=1) return(ERROR);

    /*
     * Check syntax of block length byte pair; that the bits surrounding
     * the W,D bits in the high order byte of the block length byte pair
     * are 0's.
     */

    if ((next_seg_length & 0x9000) != 0) {
        internal_error(pRJ_data,0x0F,2,(unsigned int)next_seg_length,0);
        return(ERROR);
        }
    direction = (next_seg_length & 0x2000)==0?2:-2;

    /*
     * This inner loop moves the data byte-by-byte either from or to the
     * card as indicated by "flag", incrementing bytes on the card in
     * the direction indicated by "direction".
     */

    for (ibyte=0; ibyte<(next_seg_length & 0x0FFF); ibyte++) {
        if (flag==0) *next_local_addr++ = *next_seg_addr;      /* RD  */
        else         *next_seg_addr     = *next_local_addr++;  /* WRT */
        next_seg_addr += direction;
        }

    next_seg_addr_addr += 8;             /* increment to next segment */
    }

/*
 * Check the total number of bytes actually moved according to trans-
 * action data descriptor against the total data length value stored
 * in register DATA_AREA_LEN.
 */

pRJ_data->data_count = (short)(next_local_addr-pRJ_data->driv_databuf);
if (lData != pRJ_data->data_count) {
    internal_error(pRJ_data,0x0F,3,(unsigned int)lData,
        (unsigned int)pRJ_data->data_count);
    return(ERROR);
    }

return(SUCCESS);
}

rjnew_command(command, pRJ_data)
    unsigned char        command;
    struct RJ_data_type  *pRJ_data;
{
if(-1 == rjwait_CMD_Z(pRJ_data)) {
    internal_error(pRJ_data,0x10,1,(unsigned int)command,0);
    return(ERROR);
    }
*(COMMAND + aDev) = command;
return(SUCCESS);
}

                  /********************************/
                  /********************************/
                  /*    ROUTINES INVOKED FROM     */
                  /*     HP-UX USER INTERFACE     */
                  /********************************/
                  /********************************/

  
/*
 * Notes on system intfc with (rje)open/close/read/write/ioctl:
 *
 * All communication with user space and the user structure is done
 * through uiomove.
 *
 * All the intrinsics test the bus addr to determine whether to use the
 * regular port, or the trace port for that selectcode.  A bus addr of 0
 * designates the regular port, and any other bus addr designates the
 * trace port.
 */

rje_open(dev,flag)
    dev_t  dev;
    int    flag;
{
register char        SC;
unsigned char        reset_reg;
int                  RC;
struct RJ_data_type  *pRJ_data;

SC= m_selcode(dev);
if ((isc_table[SC] == NULL) || (SC < MINISC ) || (SC >= EXTERNALISC))
    return(ENXIO);
    
if( m_busaddr(dev) ) pRJ_data = mpSCRJ_stat[SC].port1; /* Trace port  */
else                 pRJ_data = mpSCRJ_stat[SC].port0; /* Regular port*/

if (pRJ_data == NULL) return(EIO);

pRJ_data->errornum = 0;
if (pRJ_data->driverstate) {                           /* Device busy */
    internal_error(pRJ_data,0x11,3,(unsigned int)pRJ_data->driverstate,0);
    pRJ_data->errornum = 1;
    return(EBUSY);
    }
mpSCRJ_stat[SC].port1->FATAL = 0;  /* Clear any fatal err indication  */
mpSCRJ_stat[SC].port0->FATAL = 0;

aDev                       = (unsigned char *)(isc_table[SC]->card_ptr);
pRJ_data->read_type_state  = 1;
pRJ_data->write_type_state = 2;
pRJ_data->read_subf_state  = 0;
pRJ_data->write_subf_state = 0;
pRJ_data->rqb_misc_bits    = 0x80;

/*
 * Read firmware id from reset register and check is against hard-
 * coded value 0x34.
 *
 * +-0-+-1-+-2-+-3-+-4-+-5-+-6-+-7-+  Firmware Id = 0110100 = 0x34
 * |--      Firmware Id      --| r |  r: Set on write = reset card
 * +---+---+---+---+---+---+---+---+     Set on read  = card is the re-
 *                                              mote control interface.
 */

RC = SUCCESS;
if ((*(RESET_REG + aDev) & FIRMWARE_ID_MASK)!=FIRMWARE_ID) RC = ERROR;
else

    /*
     * First user resets card and copies in self_test and test_connector
     * error codes.
     */

    if ((mpSCRJ_stat[SC].nusers==0) &&
        ((RC = rjreset(pRJ_data))==SUCCESS)) {
            pRJ_data->self_test      = *(SELFTEST_ERR_CODE + aDev);
            pRJ_data->test_connector = *(TEST_CONNECTOR    + aDev);
            }

if (RC==SUCCESS) {
    pRJ_data->driverstate   = 8;
    mpSCRJ_stat[SC].nusers += 1;
    RJE_ENABLE_INT(aDev);
    return(SUCCESS);
    }
else {
    pRJ_data->errornum    = 1;
    pRJ_data->driverstate = 0;
    internal_error(pRJ_data,0x11,4,(unsigned int)*(RESET_REG+aDev),0);
    return(EIO);
    }
}

rjreset(pRJ_data)
    struct RJ_data_type  *pRJ_data;
{
unsigned char  *rqb_adr;
unsigned int   dummy;
struct timeval timer;

*(RESET_REG + aDev) = RESET_CARD_BIT;      /*Reset card*/
timer = time;
bumptime(&timer,1000000);

/*
 * This loop waits for reset to finish and for card to interrupt.
 */

while ((*(INT_REG + aDev) & INT_REQ_BIT) == 0)
    if (timercmp(&time, &timer, >)) {
        internal_error(pRJ_data,0x12,1,(unsigned int)*(INT_REG+aDev),0);
        return(ERROR);
        }

dummy = *(INT_COND + aDev);            /* Reset interrupt line */
return(SUCCESS);
}

rje_close(dev,flag)
    dev_t  dev;
    int    flag;
{
struct RJ_data_type  *pRJ_data;
unsigned int         SC;

SC = m_selcode(dev);

if ( m_busaddr(dev) ) pRJ_data = mpSCRJ_stat[SC].port1; /* Trace port */
else                  pRJ_data = mpSCRJ_stat[SC].port0; /*Regular port*/

pRJ_data->driverstate = 0;
if(mpSCRJ_stat[SC].nusers > 0) mpSCRJ_stat[SC].nusers -= 1;
if(mpSCRJ_stat[SC].nusers == 0)
    return(rjreset(pRJ_data));                     /* Last one resets */
}

rje_read(dev,uio)
dev_t       dev;
struct uio  *uio;
{
unsigned             nbytes;
struct RJ_data_type  *pRJ_data;
int                  SC;
int                  error;

SC = m_selcode(dev);
if (m_busaddr(dev)) pRJ_data = mpSCRJ_stat[SC].port1;  /* Trace port  */
else                pRJ_data = mpSCRJ_stat[SC].port0;  /* Regular port*/
pRJ_data->data_count = 0;

/*
 * Trying to read too many bytes?
 */

if ((unsigned)uio->uio_resid > MAX_RDWR_SIZE ) return(EIO);
else nbytes = (unsigned)uio->uio_resid;

pRJ_data->xaction.request_code = pRJ_data->read_type_state;
pRJ_data->xaction.func_code    = pRJ_data->read_subf_state;
pRJ_data->xaction.port_id      = pRJ_data->port_id;
pRJ_data->xaction.data_length  = nbytes;

if (rjinitiate_transaction(pRJ_data) < 0) return(gerror);

/*
 * rjcard_data_move will fill up pRJ_data->driv_databuf & determine
 * pRJ_data-> data_count.
 */

if (pRJ_data->data_count > nbytes) {       /* Trying to read too many */
    internal_error(pRJ_data,0x14,1,(unsigned int)nbytes,
        (unsigned int)pRJ_data->data_count);
    return(EIO);
    }

/*
 * uiomove returns 0 on success or "errno" value on failure, will adjust
 * uio_resid.
 */

if (error = uiomove( pRJ_data->driv_databuf, pRJ_data->data_count, UIO_READ, uio ))
    return(error);

return(SUCCESS);
}

rje_write(dev,uio)
    dev_t       dev;
    struct uio  *uio;
{
unsigned             nbytes;
struct RJ_data_type  *pRJ_data;
int                  SC;
int                  error;

SC = m_selcode(dev);
if (m_busaddr(dev)) pRJ_data = mpSCRJ_stat[SC].port1;  /* Trace port */
else                pRJ_data = mpSCRJ_stat[SC].port0;  /* Regular port*/

pRJ_data->data_count = 0;

/*
 * Trying to write too many?
 */

if ( (unsigned)uio->uio_resid > MAX_RDWR_SIZE ) return(EIO);
else nbytes = (unsigned)uio->uio_resid;

pRJ_data->xaction.request_code = pRJ_data->write_type_state;
pRJ_data->xaction.func_code    = pRJ_data->write_subf_state;
pRJ_data->xaction.port_id      = pRJ_data->port_id;
pRJ_data->xaction.data_length  = nbytes;

/*
 * uiomove returns 0 on success or "errno" value on failure, will adjust
 * uio.resid.
 */

if ( error = uiomove( pRJ_data->driv_databuf, nbytes, UIO_WRITE, uio ) )
    return(error);

if (rjinitiate_transaction(pRJ_data)==ERROR) return(gerror);
    
return(SUCCESS);
}

rje_ioctl(dev, cmd, arg, mode)
dev_t           dev;
int             cmd;
unsigned char   *arg;
int             mode;
{
unsigned char        *loc;
unsigned char        contents;
unsigned char        req;
int                  i;
int                  j;
int                  SC;
struct RJ_data_type  *pRJ_data;

SC = m_selcode(dev);
if ( m_busaddr(dev) ) pRJ_data = mpSCRJ_stat[SC].port1;        /* Trace port   */
else                  pRJ_data = mpSCRJ_stat[SC].port0;        /* Regular port */

req = cmd & 0xFF;
switch( cmd )  {
    case RJE_CONT_LINK:
    case RJE_CONT_PDI:
    case RJE_CONT_TRACE:
        pRJ_data->xaction.request_code = req;
        pRJ_data->xaction.func_code    = *(arg+1);
        pRJ_data->xaction.port_id      = pRJ_data->port_id; 
        pRJ_data->xaction.data_length  = 0;
        if (rjinitiate_transaction(pRJ_data) == ERROR) return(gerror);
        break;

    case RJE_READ: 
        pRJ_data->read_type_state = (unsigned short) *arg;
        pRJ_data->read_subf_state = (unsigned short) *(arg+1);
        break;

    case RJE_WRITE: 
        pRJ_data->write_type_state = (unsigned short) *arg;
        pRJ_data->write_subf_state = (unsigned short) *(arg+1);
        break;

    case RJE_REP_TRANS: 
        rjreport_last_xstatus(req,arg,pRJ_data);
        break;

    case RJE_REP_ERROR:
        for (i=0; i<5; i++ )
            for (j=0; j<11; j++ )
                *(arg+(11*i+j))=pRJ_data->internal_error[i][j];
        break;

    case RJE_SELF_TEST:
        *arg     = pRJ_data->self_test;
        *(arg+1) = pRJ_data->test_connector;
        break;

    default:
             return( EINVAL );

    }

return(SUCCESS);
}

rjreport_last_xstatus(req,arg,pRJ_data)
unsigned char        req;
unsigned char        *arg;
struct RJ_data_type  *pRJ_data;
{
int i;
int index;

index    = *arg;
*(arg+0) = pRJ_data->last_RSB[index].status;
*(arg+1) = pRJ_data->last_RSB[index].transfer_length & 0x00FF;
*(arg+2) = pRJ_data->last_RSB[index].transfer_length >>8;
for (i=3; i<7; i++) *(arg+i) = pRJ_data->last_RSB[index].other_data[i-3];

return(SUCCESS);
}

rjwait_CMD_Z(pRJ_data)
struct RJ_data_type  *pRJ_data;

{
struct timeval timer;
 
timer = time;
bumptime(&timer,1000000);

while (*(aDev + COMMAND) != 0)
    if (timercmp(&time, &timer, >)) {
        internal_error(pRJ_data,0x19,1,(unsigned int)lbolt,0);
        return(ERROR);
        }

return(SUCCESS);
}

unsigned char *rjwait_RQBA_NZ(pRJ_data)
struct RJ_data_type      *pRJ_data;

{
unsigned char  *value;
unsigned long  timer;
unsigned short raw_addr;

/*
 * "lbolt" (sys/systm.h) counts real time in 1/60th second intervals.
 */

timer    = lbolt;
raw_addr = 0;

/*
 * The outer loop allows the driver 4/60ths of a second to get the a non
 * zero addr for the next available request block structure on the card.
 */

while (lbolt-timer < 4) {

    /*
     * The inner loop allows the driver between 2/60ths of a second to
     * gain access to shared memory through the semaphore register (this
     * semaphore is set by the card while RQB memory management is being
     * done).  The combined action of the inner and outer loops causes
     * the driver to return an error if it cannot gain memory access in
     * 2/60 second, or if it can get memory access, it will still return
     * an error if it doesn't get a nonzero address w/in 4/60 sec (card
     * will return a 0 addr when it is out of memory).
     */

    while ( (*(aDev+CARD_SEMA) & SEMA_ENAB_BIT) != 0)
        if (lbolt-timer > 2) {
            internal_error(pRJ_data,0x1A,1,(unsigned int)lbolt,0);
            return(SUCCESS);
            }

    /*
     * Semaphore register was cleared by card, get request block struct
     * address on card and clear semaphore.
     */

    raw_addr          = twobytevalue(REQ_BLK_ADR, aDev);
    *(aDev+CARD_SEMA) = 0x00;
    if(raw_addr != 0) break;   /* Valid address retrieved, otherwise, */
                               /* try again if any of 4/60 sec left.  */
    } /* end outer loop */

if (raw_addr == 0) {           /* Timeout detected.                   */
     internal_error(pRJ_data,0x1A,2,(unsigned int)lbolt,0);
     return(SUCCESS);
     }

value = z80(raw_addr);

if (testr(value,1)!=1) {       /* Test address for bus error.         */
    internal_error(pRJ_data,0x1A,3,(unsigned int)value,0);
    return(SUCCESS);
    }

 return(value);
}

/*
 * transaction pRJ_data->xaction has already been specified by ioctl
 * call, transaction id needs only to be mapped in.
 */

rjinitiate_transaction(pRJ_data)
    struct RJ_data_type      *pRJ_data;
{
unsigned char *rqb_area;
int           i, iRSB;

if (pRJ_data->FATAL) return(ERROR);         /* Fatal error prevailing */

pRJ_data->xaction.transaction_id =
    (short) ((pRJ_data->port_id<<5)
            + rjcompute_TId(pRJ_data->xaction.request_code));

/* GCS 5/8
 * Initiate the last_RSB[] for this transaction id to 0's.  The problem
 * was that last_RSB[] was only being updated in rjeintr, therefore, if
 * an error was detected from this side and an RJE_REP_TRANS done by the
 * user, he would get the status of the previous transaction, not the
 * one he is trying to initialize.  I would like to return an error val-
 * ue, but those are defined by the card for different cases, therefore,
 * by initializing to 0 the user should consider that an error on a
 * read/write/ioctl call with a "no error" status from an ensuing
 * RJE_REP_TRANS call as an undefined I/O error.
 */

iRSB = (int)pRJ_data->xaction.request_code;
pRJ_data->last_RSB[iRSB].status          = 0x0;
pRJ_data->last_RSB[iRSB].transfer_length = 0;
for (i=0; i<4; i++) pRJ_data->last_RSB[iRSB].other_data[i] = 0x0;

/*
 * Retrieve address of request block structure on card.  An error is
 * trapped if the card cannot give us a request block structure in 2/60
 * second.  The only delay encountered should be while the Z80 is upda-
 * ting the RQB structure list.  The RQB is not used by the card until
 * it receives a COMMAND of 4 (NEW_REQ_AVAIL), so the fact that we are
 * storing data in it before the card uses it should not cause problems
 * if we later do not get the COMMAND byte.
 */

if ((rqb_area = rjwait_RQBA_NZ(pRJ_data))==0) {
    gerror = ENOSPC;
    return(ERROR);
    }

/*
 * GCS 4/4, driver did not check validity of request block given
 * to it by card
 */

if (rjcopy_RQB_to_card(pRJ_data,rqb_area) == ERROR) {
    gerror = EFAULT;
    return(ERROR);
    }

/*
 * Write new request to card and block read with a sleep until card
 * interrupts us (rjeintr).
 */

if (rjinitiator_stop(pRJ_data) == ERROR) {
    gerror = EIO;
    return(ERROR);
    }

pRJ_data->driverstate = 8;
return(SUCCESS);
}

rjinitiator_stop(pRJ_data)
struct RJ_data_type  *pRJ_data;
{                                               /* GCS 5/8  rewritten */
int r;
int old_level;
struct timeval timer;

timer = time;                         /* Save current time + 1/2 sec. */
bumptime(&timer,1000000);

r = FALSE;
while (timercmp(&time, &timer, <)) {            /* Loop until timeout */

/***********************************************************************
*                                                                      *
* <<<<<<<<<<<<<<<<<<<<     START CRITICAL SECTION     >>>>>>>>>>>>>>>> *
*                                                                      *
************************************************************************
*                                                                      *
* NOTE:  Until sleep() is executed rjeintr() is blocked               */

    old_level = spl6();  /* Set to max possible int level set on card */
    if (*(aDev + COMMAND) == 0) {
        pRJ_data->semafore    = (int)pRJ_data->xaction.request_code;
        pRJ_data->driverstate = rjcompute_TId(pRJ_data->semafore);
        *(aDev + COMMAND)     = NEW_REQ_AVAIL;

/*
 * The loop around sleep is to prevent problems in case more than one
 * process is sleeping on the same address, namely &pRJ_data->semafore.
 * Rj->semafore is set > 0 here in rjinitiator_stop, rjreset_initiator
 * sets it <= 0.
 */

        while (pRJ_data->semafore > 0)
            sleep((caddr_t) &pRJ_data->semafore, RJE_PRIORITY);
        r = TRUE;
        }
    splx(old_level);

/***********************************************************************
*                                                                      *
* <<<<<<<<<<<<<<<<<<<<<     END CRITICAL SECTION     >>>>>>>>>>>>>>>>> *
*                                                                      *
***********************************************************************/

    if (r) return(pRJ_data->semafore);  /* GCS 3/27, error reporting  */
    } /* end while */

internal_error(pRJ_data,0x19,1,(unsigned int)lbolt,0);
return(ERROR);          /* While-loop exit == timer pop, COMMAND != 0 */
}

rjcopy_RQB_to_card(pRJ_data,rqba)
    struct RJ_data_type  *pRJ_data;
    unsigned char        *rqba;
{
int            i;
unsigned char  *temp;

if (testr(rqba,1)!=1) return(ERROR); /* GCS 4/4, ck card RQB addr */
*rqba=(pRJ_data->xaction.transaction_id & 0xFF);
rqba +=2;
*rqba=(pRJ_data->xaction.transaction_id & 0xFF00) >>8;
rqba +=2;
*rqba= (pRJ_data->xaction.request_code) | pRJ_data->rqb_misc_bits;
rqba +=2;
*rqba= pRJ_data->xaction.func_code;
rqba +=2;
*rqba= pRJ_data->xaction.port_id;
rqba +=2;
*rqba=(pRJ_data->xaction.data_length & 0xFF);
rqba +=2;
*rqba=(pRJ_data->xaction.data_length & 0xFF00) >>8;
return(SUCCESS);
}

internal_error(pRJ_data, funcno, errno, other1, other2)
struct RJ_data_type  *pRJ_data;
unsigned char        funcno;
unsigned char        errno;
unsigned int         other1;
unsigned int         other2;
{
unsigned int  i;
unsigned int  j;

for (i=4; i>0; i--)
    for (j=0; j<11; j++)
        pRJ_data->internal_error[i][j] = pRJ_data->internal_error[i-1][j];
pRJ_data->internal_error[0][0]  = funcno;
pRJ_data->internal_error[0][1]  = errno;
pRJ_data->internal_error[0][2]  = (unsigned char) pRJ_data->driverstate;
pRJ_data->internal_error[0][3]  = (other1 & 0xFF000000) >> 24;
pRJ_data->internal_error[0][4]  = (other1 & 0x00FF0000) >> 16;
pRJ_data->internal_error[0][5]  = (other1 & 0x0000FF00) >>  8;
pRJ_data->internal_error[0][6]  = (other1 & 0x000000FF);
pRJ_data->internal_error[0][7]  = (other2 & 0xFF000000) >> 24;
pRJ_data->internal_error[0][8]  = (other2 & 0x00FF0000) >> 16;
pRJ_data->internal_error[0][9]  = (other2 & 0x0000FF00) >>  8;
pRJ_data->internal_error[0][10] = (other2 & 0x000000FF);
return(SUCCESS);
}

/*
 * linking and initialization routines called at powerup.
 */

#define iSC pisc_table_type->my_isc

extern int (*make_entry)();
int (*rje_saved_make_entry)();

rje_make_entry(id,pisc_table_type)
register struct isc_table_type *pisc_table_type;
{
if (id == 20+32)
    if (data_com_type(pisc_table_type) == 99) {      
        isrlink(rjeintr,                                /* ISR               */
            pisc_table_type->int_lvl,                   /* interrupt level   */
            (char *)pisc_table_type->card_ptr+INT_REG,  /* interrupt.regaddr */
            0xC0, 0xC0, 0,                              /* misc mask values  */
            0);                                         /* interrupt.temp    */
	pisc_table_type->card_type = HP98641;
        mpSCRJ_stat[iSC].nusers = 0;
        mpSCRJ_stat[iSC].port0 = (struct RJ_data_type *)
            sys_memall(sizeof(struct RJ_data_type));

        if (mpSCRJ_stat[iSC].port0) {
            mpSCRJ_stat[iSC].port0->iocard = (unsigned char *)((iSC + 0x60) << 16);
            mpSCRJ_stat[iSC].port0->driverstate = 0;
            mpSCRJ_stat[iSC].port0->port_id = 0;
            }
        else
            printf("Warning: not enough mem for HP98641\n");

        mpSCRJ_stat[iSC].port1 = (struct RJ_data_type *)
            sys_memall(sizeof(struct RJ_data_type));

        if (mpSCRJ_stat[iSC].port1) {
            mpSCRJ_stat[iSC].port1->iocard = (unsigned char *)((iSC + 0x60) << 16);
            mpSCRJ_stat[iSC].port1->driverstate = 0;
            mpSCRJ_stat[iSC].port1->port_id = 1;
            } 
        return (io_inform("HP98641 RJE interface", pisc_table_type, 0));
        }
return (*rje_saved_make_entry)(id, pisc_table_type);
}

/**********************************************************************/
rje_link()
{
rje_saved_make_entry = make_entry;
make_entry           = rje_make_entry;
}
