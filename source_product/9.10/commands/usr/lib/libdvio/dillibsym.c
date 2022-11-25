/* @(#) $Revision: 70.1 $ */   
#include <sys/param.h>
#include <errno.h>
#include <sys/timeout.h>
#include <sys/hpibio.h>
#include <sys/buf.h>
#include <sys/iobuf.h>
#include <sys/ti9914.h>
#include <sys/hshpib.h>
#include <sys/dilio.h>
#include <sys/dil.h>

main()
{
	struct isc_table_type *sc = (struct isc_table_type *)0;
	struct buf *bp = (struct buf *)0;
	struct fd_info *fdinfo = (struct fd_info *)0;
	struct iobuf *iob = (struct iobuf *)0;
	struct TI9914 *tip = (struct TI9914 *)0;
	struct simon *sip = (struct simon *)0;

	/* simon declarations */
	printf("set	PHI_INTR,0x%x\n", &sip->med_intr);
	printf("set	PHI_IMSK,0x%x\n", &sip->med_imsk);
	printf("set	PHI_FIFO,0x%x\n", &sip->med_fifo);
	printf("set	PHI_STATUS,0x%x\n", &sip->med_status);
	printf("set	PHI_CTRL,0x%x\n", &sip->med_ctrl);
	printf("set	SIM_CTRL,0x%x\n", &sip->sim_ctrl);

	printf("set	P_INIT_FIFO,0x%x\n", M_INIT_FIFO);
	printf("set	P_FIFO_ROOM,0x%x\n", M_FIFO_ROOM);
	printf("set	P_FIFO_BYTE,0x%x\n", M_FIFO_BYTE);
	printf("set	P_FIFO_IDLE,0x%x\n", M_FIFO_IDLE);
	printf("set	P_ROOM_IDLE,0x%x\n", M_FIFO_ROOM | M_FIFO_IDLE);
	printf("set	P_FIFO_EOI,0x%x\n", M_FIFO_EOI);
	printf("set	P_FIFO_ATN,0x%x\n", M_FIFO_ATN);
	printf("set	P_FIFO_LF_INH,0x%x\n", M_FIFO_LF_INH);
	printf("set	P_FIFO_UCT_XFR,0x%x\n", M_FIFO_UCT_XFR);
	printf("set	P_HPIB_CTRL,0x%x\n", M_HPIB_CTRL);
	printf("set	P_INT_ENAB,0x%x\n", M_INT_ENAB);
	printf("set	P_REN,0x%x\n", M_REN);
	printf("set	P_NOT_REN,0x%x\n", ~M_REN);
	printf("set	P_IFC,0x%x\n", M_IFC);
	printf("set	P_NOT_IFC,0x%x\n", ~M_IFC);
	printf("set	CLR_IDLE_ROOM,0x%x\n", ~(M_FIFO_ROOM | M_FIFO_IDLE));
	printf("set	CLR_IDLE,0x%x\n", ~M_FIFO_IDLE);
	printf("set	CLR_BYTE,0x%x\n", ~M_FIFO_BYTE);
	printf("set	CLR_ROOM,0x%x\n", ~M_FIFO_ROOM);
	printf("set	S_ENAB,0x%x\n", S_ENAB);
	printf("set	MA,0x%x\n", MA);

	printf("set	INT_LVL,0x%x\n", &sc->int_lvl);
	printf("set	TRANSFER,0x%x\n", &sc->transfer);
	printf("set	CARD_PTR,0x%x\n", &sc->card_ptr);
	printf("set	OWNER,0x%x\n", &sc->owner);
	printf("set	DMA_CHAN,0x%x\n", &sc->dma_chan);
	printf("set	INTLOC,0x%x\n", &sc->intloc);
	printf("set	INTCOPY,0x%x\n", &sc->intcopy);
	printf("set	MY_ADDRESS,0x%x\n", &sc->my_address);
	printf("set	COUNT,0x%x\n", &sc->count);
	printf("set	BUFFER,0x%x\n", &sc->buffer);
	printf("set	STATE,0x%x\n", &sc->state);
	printf("set	TFR_CONTROL,0x%x\n", &sc->tfr_control);

	/* fd_info from dil.h */
	printf("set	FD_STATE,0x%x\n", &fdinfo->state);
	printf("set	FD_BA,0x%x\n", &fdinfo->ba);
	printf("set	FD_CP,0x%x\n", &fdinfo->cp);
	printf("set	FD_PATTERN,0x%x\n", &fdinfo->pattern);
	printf("set	FD_CARD_ADDRESS,0x%x\n", &fdinfo->card_address);
	printf("set	FD_REASON,0x%x\n", &fdinfo->reason);
	printf("set	FD_ADDR,0x%x\n", &fdinfo->addr);
	printf("set	FD_TEMP,0x%x\n", &fdinfo->temp);
	printf("set	FD_DEV,0x%x\n", &fdinfo->dev);
	printf("set	FD_CARD_TYPE,0x%x\n", &fdinfo->card_type);
	printf("set	FD_D_SC,0x%x\n", &fdinfo->d_sc);

	printf("set	D_16_BIT,0x%x\n", D_16_BIT);
	printf("set	EIR_CONTROL,0x%x\n", EIR_CONTROL);

	/* card constants from hpibio.h */
	printf("set	INTERNAL_HPIB,0x%x\n", INTERNAL_HPIB);
	printf("set	HP98624,0x%x\n", HP98624);
	printf("set	HP98625,0x%x\n", HP98625);
	printf("set	HP98622,0x%x\n", HP98622);

	/* termination reasons from dil.h */
	printf("set	TR_COUNT,0x%x\n", TR_COUNT);
	printf("set	TR_MATCH,0x%x\n", TR_MATCH);
	printf("set	TR_END,0x%x\n", TR_END);

	printf("set	MARKSTACK,0x%x\n", &iob->markstack);
	printf("set	TIMEFLAG,0x%x\n", &iob->timeflag);

	printf("set	INTSTAT0,0x%x\n", &tip->intstat0);
	printf("set	DATAIN,0x%x\n", &tip->datain);

	return(0);
}
