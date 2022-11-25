/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/audio.c,v $
 * $Revision: 1.2.83.7 $	$Author: drew $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/11/29 08:27:37 $
 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

#include "../h/errno.h"
#include "../h/param.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/systm.h"
#include "../h/debug.h"
#ifdef __hp9000s700
#include "../h/io.h"
#include "../h/malloc.h"
#include "../h/utsname.h"
#include "../wsio/eisa.h"
#include "../sio/llio.h"
#include "../machine/pdc_rqsts.h"
#include "../machine/pde.h"
#endif
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../wsio/intrpt.h"
#include "../wsio/beeper.h"
#include "../wsio/audio.h"

#undef timeout /* Use real timeout */

#ifdef __hp9000s700
/* Perform a request for kernel dynamic memory, then zero it out. */
#define MALLOC_ETC(where, cast, size, parm1, parm2, message) \
        MALLOC((where), cast, (size), (parm1), (parm2)); \
        if ((where) == NULL) \
            panic(message); \
        bzero((where), (size))

#endif

#ifndef	TRUE
#define	TRUE		1
#define	FALSE		0
#endif

#define SINE_TABLE_SIZE 65
#define SINE_TABLE_MASK 0x3f

/* Global list of audio devices */

extern struct audio_descriptor *audio_devices;

/* default_config contains state that is set    */
/* at powerup and whenever the device is closed */

static struct psb2160conf default_config = {
    0x8888, 0x8888, 0, 0, 0, 0, 0, 0, 0x88, 0x88, 0x88, 0x88, 0, 0,
    (CR1_GX|CR1_GR|CR1_GZ), (CR2_SABCD|CR2_ELS|CR2_AM|CR2_EFC),
    (CR3_MG_28|CR3_OUT_LH3|CR3_OM_NORMAL), (CR4_TM|CR4_ULAW),
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static unsigned long default_cntl = (CNTL_DF_ULAW | CNTL_RATE_8000);
static unsigned long default_gain =
		(GAIN_HE | GAIN_LE | GAIN_SE | GAIN_IS | 0xf00186);

static int audio_probe_sc;

/* Forward Declarations */

extern unsigned short beep_freq_tab[];
extern unsigned char  beep_gain_tab[];
extern unsigned char  monitor_gain_tab[];
extern unsigned short soundbeep_gain_tab[];
extern unsigned short audio_gain_tab[];
extern unsigned short alaw_tab[];
extern unsigned short ulaw_tab[];
extern short sine_wave_table[];
extern int   sample_rate_tab[];
extern unsigned int   sample_rate_cftab[];

struct audio_descriptor *audio_get_descriptor(), *audio2_probe();
void audio1_isr(), audio_link(), audio1_init_config(), audio1_set_config();
void audio2_isr(), audio2_set_config();
void audio_rcv_on(), audio_rcv_off();
void rw_close(), sync_receive_buffer();
void audio_send_ctl();
void audio_describe(), audio1_set_gains(), audio2_set_gains();
void audio_set_output(), audio_set_format(), audio_output_drain();
void audio_set_input(), audio_set_channels(), beeper_off(), beeper_reset();
void audio_beeper(), audio1_beeper(), audio2_beeper();
void audio_dma_stop(), silence_fill(), beep_fill();
void audio2_meter_sum(), audio2_input_drain();
void audio_free_buffers();
int audio_set_sample_rate(), audio_set_beep();
int audio_test_mode(), audio_test_rw_register(), audio_test_rw_buffer();
int audio2_get_irq();
int audio_beeper_time();

/* for configurability */
extern void (*audiobeeper)();
extern int (*audiobeepertime)();

audio_open(dev, flag)
    dev_t dev;
    int flag;
{
    register struct audio_descriptor *audio_des;
    register struct audio2_reg *audio2_regs;
    register int i;
    int openflags;

    if ((audio_des = audio_get_descriptor(m_selcode(dev))) == 0) {

	/*
	 * Although not really supported, it is possible to put
	 * a PA upgrade Timber-C/Audio card into a 720,730 or 750.
	 * However, since PDC on those machines are not aware of
	 * the audio, it won't be found. audio2_probe() checks for
	 * the existance of the hardware, and if found, configures it
	 * and sets it up.
	 */

	if ((audio_des = audio2_probe(dev)) == 0)
	    return(ENODEV);
    }


    if (audio_des->open_count == 0) {

	/*
	 * First open. Reset configuration. If AUD_F_BEEPCONF is
	 * set, let beeper_reset do the work, otherwise, we do it
	 * here.
	 */

	if (audio_des->flags & AUD_F_BEEPCONF)
	    beeper_reset(audio_des,TRUE);
	else {
	    if (audio_des->audio2flag) {

		audio_dma_stop(audio_des);

		audio2_regs = (struct audio2_reg *)audio_des->audio_regs;
		audio2_regs->gainctl = default_gain;
		audio_des->cur_gainctl = default_gain;
		audio2_set_config(audio_des,default_cntl);
	    }
	    else
		audio1_set_config(audio_des,&default_config);
	}
    }

    if (IS_CTL(dev))
	openflags = AUD_F_OPEN;
    else {

	/* Only one open allowed at a time */

	if (audio_des->flags & AUD_F_RWOPEN)
	    return(EBUSY);

	/* Set format from minor number */

	i = dev & 0xf;
	if (i != 0) {

	    i--;

	    if (i > AUDIO_FORMAT_LINEAR16BIT)
		return(ENXIO);

	    audio_set_format(audio_des,i);

	    /* Set output from minor number */

	    i = (dev >> 4) & 0xf;
	    if (i > AUDIO_OUT_LINE)
		return(ENXIO);

	    if (audio_des->audio2flag) {
		if (i != AUDIO_OUT_LINE)
		    i = 3 - i;
	    }
	    else {
		if (i == AUDIO_OUT_LINE)
		    return(ENXIO);

		i = 3 - i;
	    }

	    audio_set_output(audio_des, i);
	}

	openflags = (AUD_F_RWOPEN|AUD_F_OPEN);
    }

    i = CRIT();
    audio_des->flags |= openflags;
    UNCRIT(i);
    audio_des->open_count++;

    return(0);
}

audio_close(dev, flag)
    dev_t dev;
    int flag;
{
    register struct audio_descriptor *audio_des;
    register struct audio1_reg *audio1_regs;
    register int i;

    if ((audio_des = audio_get_descriptor(m_selcode(dev))) == 0)
	return(ENODEV);

    if (audio_des->open_count <= 0)
	return(ENODEV); /* This should never happen! */

    /* If this is the read/write device, drain and release buffers */

    if (!IS_CTL(dev))
	rw_close(audio_des);

    if (audio_des->open_count == 1) {

	/*
	 * The buffers might not have been freed in rw_close if
	 * AUD_F_BUFVIACTL was set. So make sure they are freed
	 * here.
	 */

	audio_free_buffers(audio_des);

	i = CRIT();

	/* reset registers and flags */

	if (!audio_des->audio2flag) {
	    audio1_regs = (struct audio1_reg *)audio_des->audio_regs;
	    audio1_regs->fifointctl = IN_HALF_INT_ENA;
	    audio1_regs->interrupt  = INT_ENA;
	}

	/*
	 * Only preserve AUD_F_SOUNDBEEP, AUD_F_BEEPCONF & AUD_F_NOBEEPER
	 * across last close.
	 */

	audio_des->flags &= (AUD_F_SOUNDBEEP|AUD_F_NOBEEPER|AUD_F_BEEPCONF);
	UNCRIT(i);

	audio_des->cur_gain[0].receive_gain = 0;
	audio_des->cur_gain[1].receive_gain = 0;
	audio_des->cur_gain[0].transmit_gain = 0;
	audio_des->cur_gain[1].transmit_gain = 0;
	audio_des->cur_gain[0].monitor_gain = AUDIO_OFF_GAIN;
	audio_des->cur_gain[1].monitor_gain = AUDIO_OFF_GAIN;
	audio_des->audio_output = (AUDIO_OUT_INTERNAL|AUDIO_OUT_EXTERNAL);
	audio_des->audio_input = AUDIO_IN_MIKE;
	audio_des->audio_format = AUDIO_FORMAT_ULAW;
	audio_des->audio_sample_rate = 8000;
	audio_des->rcv_overflow_cnt  = 0;
	audio_des->snd_underflow_cnt = 0;
	audio_des->receive_nsamples = 0;
	audio_des->receive_sample_sum[0] = audio_des->receive_sample_sum[1] = 0;
	audio_des->receive_sample_peak[0] = audio_des->receive_sample_peak[1] = 0;
	audio_des->receive_sample_save  = 0;
	audio_des->transmit_nsamples = 0;
	audio_des->transmit_sample_sum[0] = audio_des->transmit_sample_sum[1]  = 0;
	audio_des->transmit_sample_peak[0] = audio_des->transmit_sample_peak[1] = 0;
	audio_des->transmit_sample_save = 0;
	audio_des->receive_meter_state  = METER_OFF;
	audio_des->transmit_meter_state = METER_OFF;

	if (audio_des->audio2flag)
	    audio_des->rcv_extra_ptr = audio_des->rcv_extra_buffer;
    }

    audio_des->open_count--;
    return(0);
}

void
rw_close(audio_des)
    register struct audio_descriptor *audio_des;
{
    register struct audio1_reg *audio1_regs;
    register struct audio2_reg *audio2_regs;
    register int i;

    i = CRIT();

    /* disable receive operation. */

    audio_rcv_off(audio_des);
    if (audio_des->audio2flag) {

	audio2_regs = (struct audio2_reg *)audio_des->audio_regs;

	if (audio2_regs->dstatus & (DSTAT_IE|DSTAT_PC|DSTAT_RC)) {

	    audio_output_drain(audio_des,PSWP);

	    audio_des->flags &= ~(AUD_F_SNDON|AUD_F_SNDLASTPG);
	}
	UNCRIT(i);
    }
    else {

	/* Enable the output fifo half empty interrupt request */

	audio1_regs = (struct audio1_reg *)audio_des->audio_regs;
	audio1_regs->fifointctl |= OUT_HALF_INT_ENA;
	UNCRIT(i);
	audio_output_drain(audio_des,PSWP);
    }

    /* Clear RW_OPEN flag */

    i = CRIT();
    audio_des->flags &= ~AUD_F_RWOPEN;
    UNCRIT(i);

    if ((audio_des->flags & AUD_F_BUFVIACTL) == 0) {

	/* Free buffers */

	audio_free_buffers(audio_des);
    }

    return;
}

void
audio_free_buffers(audio_des)
    register struct audio_descriptor *audio_des;
{
    if (audio_des->rcv_bufsize != 0)
	sys_memfree(audio_des->rcv_buffer,audio_des->rcv_bufsize);
    if (audio_des->snd_bufsize != 0)
	sys_memfree(audio_des->snd_buffer,audio_des->snd_bufsize);

    if (audio_des->flags & AUD_F_TESTMODE) {
	sys_memfree(audio_des->test_buffer,MAX_TEST_BUFS * NBPG);
	audio_des->flags &= ~AUD_F_TESTMODE;
    }

    /* Reset buffer related variables */

    audio_des->rcv_bufsize  = 0;
    audio_des->rcv_bufcount = 0;
    audio_des->rcv_buffer   = (unsigned char *)0;
    audio_des->rcv_buftail  = (unsigned char *)0;
    audio_des->rcv_bufhead  = (unsigned char *)0;
    audio_des->snd_bufsize  = 0;
    audio_des->snd_bufcount = 0;
    audio_des->snd_buffer   = (unsigned char *)0;
    audio_des->snd_buftail  = (unsigned char *)0;
    audio_des->snd_bufhead  = (unsigned char *)0;
    audio_des->test_buffer  = (unsigned char *)0;

    audio_des->rcv_sel_threshold = DEF_SEL_THRESHOLD;
    audio_des->snd_sel_threshold = DEF_SEL_THRESHOLD;
    audio_des->audio_rsel = (struct proc *)0;
    audio_des->audio_wsel = (struct proc *)0;

    return;
}

audio_ioctl(dev,request,arg)
    int dev;
    int request;
    register int *arg;
{
    register int i;
    register int j;
    register struct audio_descriptor *audio_des;
    register struct audio1_reg *audio1_regs;
    register struct audio2_reg *audio2_regs;
    struct psb2160conf *new1_config_ptr;
    struct cs4215conf *new2_config_ptr;
    struct audio_describe *adptr;
    struct audio_limits *alimptr;
    struct audio_gain *gainptr;
    struct audio_beeper_test *abtestptr;
    struct audio_select_thresholds *astptr;
    struct audio_status *astatptr;
    struct audio_datainfo *adiptr;
    struct audio_meter_ctl *amcptr;
    struct audio_beep_type *abtptr;
    struct beep_info *biptr;
    unsigned char *bufptr;
    unsigned long meter_offset;

    if ((audio_des = audio_get_descriptor(m_selcode(dev))) == 0)
	return(ENODEV);

    if (audio_des->audio2flag)
	audio2_regs = (struct audio2_reg *)audio_des->audio_regs;
    else
	audio1_regs = (struct audio1_reg *)audio_des->audio_regs;

    if (   request != AUDIO_TEST_MODE && request != AUDIO_TEST_RW_REGISTER
	&& request != AUDIO_TEST_RW_BUFFER && request != AUDIO_TEST_INT) {

	if (audio_des->flags & AUD_F_TESTMODE)
	    return(EACCES);
    }

    /* disable any beeping on this device and/or restore config */

    if (audio_des->flags & AUD_F_BEEPCONF)
	beeper_reset(audio_des,FALSE);

    switch(request) {

    case AUDIO_DESCRIBE:
	adptr = (struct audio_describe *)arg;
	audio_describe(audio_des,adptr);
	break;

    case AUDIO_SET_LIMITS:

	if (!suser())
	    return(EPERM);

	alimptr = (struct audio_limits *)arg;
	if (   alimptr->max_receive_buffer_size  < audio_des->min_bufsize
	    || alimptr->max_transmit_buffer_size < audio_des->min_bufsize) {

	    return(EINVAL);
	}

	audio_des->max_rcv_bufsize = alimptr->max_receive_buffer_size;
	audio_des->max_snd_bufsize = alimptr->max_transmit_buffer_size;
	break;

    case AUDIO_GET_LIMITS:
	alimptr = (struct audio_limits *)arg;
	alimptr->max_receive_buffer_size  = audio_des->max_rcv_bufsize;
	alimptr->max_transmit_buffer_size = audio_des->max_snd_bufsize;
	break;

    case AUDIO_SET_RXBUFSIZE:
	i = *arg;
	if (i < audio_des->min_bufsize || i > audio_des->max_rcv_bufsize)
	    return(EINVAL);

	/* If audio II, make sure size is page multiple */

	if (audio_des->audio2flag && (i & (NBPG -1)) != 0)
	    return(EINVAL);

	if (i > audio_des->rcv_bufsize) {

	    j = CRIT();
	    if (audio_des->rcv_bufsize > 0) {
		if (audio_des->rcv_bufcount != 0) {
		    UNCRIT(j);
		    return(EBUSY);
		}

		sys_memfree(audio_des->rcv_buffer,audio_des->rcv_bufsize);
		audio_des->rcv_bufsize = 0;
	    }

	    /* Allocate receive buffer */

	    bufptr = (unsigned char *)sys_memall(i);
	    if (bufptr == (unsigned char *)0) {
		UNCRIT(j);
		return(ENOMEM);
	    }

#ifdef __hp9000s700
	    if (audio_des->audio2flag) {
		pdcache(KERNELSPACE,bufptr,i);
		_SYNC();
	    }
#endif

	    audio_des->rcv_bufsize = i;
	    audio_des->rcv_bufcount = 0;
	    audio_des->rcv_buffer  = bufptr;
	    audio_des->rcv_bufhead = audio_des->rcv_buftail = bufptr;
	    if (IS_CTL(dev))
		audio_des->flags |= AUD_F_BUFVIACTL;

	    UNCRIT(j);
	}
	break;

    case AUDIO_SET_TXBUFSIZE:
	i = *arg;
	if (i < audio_des->min_bufsize || i > audio_des->max_snd_bufsize)
	    return(EINVAL);

	/* If audio II, make sure size is page multiple */

	if (audio_des->audio2flag && (i & (NBPG -1)) != 0)
	    return(EINVAL);

	if (i > audio_des->snd_bufsize) {

	    j = CRIT();
	    if (audio_des->snd_bufsize > 0) {
		if (audio_des->snd_bufcount != 0) {
		    UNCRIT(j);
		    return(EBUSY);
		}

		sys_memfree(audio_des->snd_buffer,audio_des->snd_bufsize);
		audio_des->snd_bufsize = 0;
	    }

	    /* Allocate send buffer */

	    bufptr = (unsigned char *)sys_memall(i);
	    if (bufptr == (unsigned char *)0) {
		UNCRIT(j);
		return(ENOMEM);
	    }

	    audio_des->snd_bufsize = i;
	    audio_des->snd_bufcount = 0;
	    audio_des->snd_buffer  = bufptr;
	    audio_des->snd_bufhead = audio_des->snd_buftail = bufptr;
	    audio_des->snd_meter_ptr = bufptr;
	    if (IS_CTL(dev))
		audio_des->flags |= AUD_F_BUFVIACTL;
	    UNCRIT(j);
	}
	break;

    case AUDIO_GET_RXBUFSIZE:
	*arg = audio_des->rcv_bufsize;
	break;

    case AUDIO_GET_TXBUFSIZE:
	*arg = audio_des->snd_bufsize;
	break;

    case AUDIO_SET_OUTPUT:
	i = *arg;
	if (i < 0 || i > (AUDIO_OUT_INTERNAL|AUDIO_OUT_EXTERNAL|AUDIO_OUT_LINE))
	    return(EINVAL);

	audio_set_output(audio_des,i);
	break;

    case AUDIO_GET_OUTPUT:
	*arg = audio_des->audio_output;
	break;

    case AUDIO_SET_INPUT:
	i = *arg;
	if (audio_des->audio2flag) {
	    if (i != AUDIO_IN_MIKE && i != AUDIO_IN_LINE)
		return(EINVAL);

	    audio_set_input(audio_des,i);
	}
	else {
	    if (i != AUDIO_IN_MIKE)
		return(EINVAL);
	}
	break;

    case AUDIO_GET_INPUT:
	*arg = audio_des->audio_input;
	break;

    case AUDIO_SET_GAINS:
	gainptr = (struct audio_gain *)arg;
	if (audio_des->audio2flag) {
	    if  (audio_des->flags & AUD_F_STEREO) {
		i = gainptr->channel_mask;
		if (i > (AUDIO_CHANNEL_0|AUDIO_CHANNEL_1))
		    return(EINVAL);
	    }
	    else {
		i = AUDIO_CHANNEL_0|AUDIO_CHANNEL_1;
		gainptr->channel_mask = i;
		gainptr->cgain[1] = gainptr->cgain[0];
	    }
	}
	else {

	    /* In the name of backwards compatibility */

	    i = AUDIO_CHANNEL_0;
	    gainptr->channel_mask = AUDIO_CHANNEL_0;
	}

	for (j = 0; j < 2; j++) {
	    if (i & (1 << j)) {
		if (   gainptr->cgain[j].receive_gain <  AUDIO_OFF_GAIN
		    || gainptr->cgain[j].receive_gain >  AUDIO_MAX_GAIN
		    || gainptr->cgain[j].transmit_gain < AUDIO_OFF_GAIN
		    || gainptr->cgain[j].transmit_gain > AUDIO_MAX_GAIN
		    || gainptr->cgain[j].monitor_gain <  AUDIO_OFF_GAIN
		    || gainptr->cgain[j].monitor_gain >  AUDIO_MAX_GAIN ) {

		    return(EINVAL);
		}
	    }
	}

	if (audio_des->audio2flag)
	    audio2_set_gains(audio_des,gainptr);
	else {
	    audio1_set_gains(audio_des,
			    gainptr->cgain[0].receive_gain,
			    gainptr->cgain[0].transmit_gain,
			    gainptr->cgain[0].monitor_gain );
	}
	break;

    case AUDIO_GET_GAINS:
	gainptr = (struct audio_gain *)arg;
	if (audio_des->audio2flag)
	    i = (AUDIO_CHANNEL_0|AUDIO_CHANNEL_1);
	else
	    i = AUDIO_CHANNEL_0;

	for (j = 0; j < 2; j++) {
	    if (i & (1 << j))
		gainptr->cgain[j]  = audio_des->cur_gain[j];
	}
	gainptr->channel_mask = i;
	break;

    case AUDIO_SET_DATA_FORMAT:
	i = *arg;
	if (i < 0 || i > AUDIO_FORMAT_LINEAR16BIT)
	    return(EINVAL);

	if (audio_des->rcv_bufcount != 0 || audio_des->snd_bufcount != 0)
	    return(EBUSY);

	if (   audio_des->audio2flag
	    && (audio2_regs->dstatus & (DSTAT_PC|DSTAT_RC)) )
	    return(EBUSY);

	audio_set_format(audio_des,i);
	break;

    case AUDIO_GET_DATA_FORMAT:
	*arg = audio_des->audio_format;
	break;

    case AUDIO_SET_SAMPLE_RATE:
	i = *arg;
	if (audio_des->audio2flag) {
	    j = audio_set_sample_rate(audio_des,i);
	    if (j != 0)
		return(j);
	}
	else {
	    if (i != 8000)
		return(EINVAL);
	}

	break;

    case AUDIO_GET_SAMPLE_RATE:
	*arg = audio_des->audio_sample_rate;
	break;

    case AUDIO_SET_CHANNELS:
	i = *arg;
	if (!audio_des->audio2flag) {
	    if (i != 1)
		return(EINVAL);
	}
	else {
	    if (i != 1 && i != 2)
		return(EINVAL);

	    if (audio_des->rcv_bufcount != 0 || audio_des->snd_bufcount != 0)
		return(EBUSY);

	    if (audio2_regs->dstatus & (DSTAT_PC|DSTAT_RC) )
		return(EBUSY);

	    audio_set_channels(audio_des,i);
	}
	break;

    case AUDIO_GET_CHANNELS:
	if (audio_des->flags & AUD_F_STEREO)
	    *arg = 2;
	else
	    *arg = 1;
	break;

    case AUDIO_SET_SEL_THRESHOLD:

	if (IS_CTL(dev))
	    return(EACCES);

	astptr = (struct audio_select_thresholds *)arg;
	if (   astptr->read_threshold < 1
	    || astptr->write_threshold < 1) {

	    return(EINVAL);
	}

	audio_des->rcv_sel_threshold = astptr->read_threshold;
	audio_des->snd_sel_threshold = astptr->write_threshold;
	break;

    case AUDIO_GET_SEL_THRESHOLD:
	astptr = (struct audio_select_thresholds *)arg;
	astptr->read_threshold  = audio_des->rcv_sel_threshold;
	astptr->write_threshold = audio_des->snd_sel_threshold;
	break;

    case AUDIO_GET_STATUS:
	astatptr = (struct audio_status *)arg;
	i = CRIT();
	if (audio_des->flags & AUD_F_RCVPAUSE)
	    astatptr->receive_status = AUDIO_PAUSE;
	else {
	    if (audio_des->flags & AUD_F_RCVON)
		astatptr->receive_status = AUDIO_BUSY;
	    else
		astatptr->receive_status = AUDIO_DONE;
	}

	if (audio_des->flags & AUD_F_SNDPAUSE)
	    astatptr->transmit_status = AUDIO_PAUSE;
	else {
	    if (audio_des->audio2flag) {
		if (audio_des->flags & AUD_F_SNDON)
		    astatptr->transmit_status = AUDIO_BUSY;
		else
		    astatptr->transmit_status = AUDIO_DONE;
	    }
	    else {
		if (   audio_des->snd_bufcount > 0
		    || (audio1_regs->fifostatus & OUT_EMPTY) == 0 ) {

		    astatptr->transmit_status = AUDIO_BUSY;
		}
		else
		    astatptr->transmit_status = AUDIO_DONE;
	    }
	}

	astatptr->receive_buffer_count = audio_des->rcv_bufcount;
	astatptr->transmit_buffer_count = audio_des->snd_bufcount;
	astatptr->receive_overflow_count = audio_des->rcv_overflow_cnt;
	astatptr->transmit_underflow_count = audio_des->snd_underflow_cnt;
	UNCRIT(i);
	break;

    case AUDIO_RESET:
	i = *arg;
	j = CRIT();
	if (i & RESET_OVRANGE) {
	    if (audio_des->audio2flag)
		audio2_regs->ovrange = 0;
	    else {
		UNCRIT(j);
		return(EINVAL);
	    }
	}

	if (i & RESET_RX_BUF) {
	    audio_des->flags &= ~AUD_F_RCVPAUSE;
	    audio_rcv_off(audio_des);
	    audio_des->rcv_bufcount = 0;
	    audio_des->rcv_bufhead = audio_des->rcv_buffer;
	    audio_des->rcv_buftail = audio_des->rcv_buffer;
	}

	if (i & RESET_TX_BUF) {
	    if (audio_des->audio2flag) {
		audio_des->flags &= ~AUD_F_SNDON;
		UNCRIT(j);
		audio_output_drain(audio_des,PSWP);
		j = CRIT();
		audio_des->snd_bufcount = 0;
		audio_des->snd_bufhead = audio_des->snd_buffer;
		audio_des->snd_buftail = audio_des->snd_buffer;
		audio_des->snd_meter_ptr = audio_des->snd_buffer;
		audio_des->flags &= ~(AUD_F_SNDPAUSE|AUD_F_SNDON|AUD_F_SNDLASTPG);
	    }
	    else {
		audio_des->snd_bufcount = 0;
		audio_des->snd_bufhead = audio_des->snd_buffer;
		audio_des->snd_buftail = audio_des->snd_buffer;
		audio_des->snd_meter_ptr = audio_des->snd_buffer;
		audio_des->flags &= ~AUD_F_SNDPAUSE;
		audio1_regs->fifointctl &= ~OUT_HALF_INT_ENA;
		UNCRIT(j);
		audio_output_drain(audio_des,PSWP);
		j = CRIT();
	    }
	}

	if (i & RESET_RX_OVF)
	    audio_des->rcv_overflow_cnt = 0;

	if (i & RESET_TX_UNF)
	    audio_des->snd_underflow_cnt = 0;

	UNCRIT(j);
	break;

    case AUDIO_PAUSE:
	i = *arg;
	if (i & AUDIO_RECEIVE) {
	    j = CRIT();
	    audio_des->flags |= AUD_F_RCVPAUSE;
	    audio_rcv_off(audio_des);
	    UNCRIT(j);
	}

	if (i & AUDIO_TRANSMIT) {
	    j = CRIT();
	    audio_des->flags |= AUD_F_SNDPAUSE;
	    if (audio_des->audio2flag)
		audio_des->flags &= ~(AUD_F_SNDON|AUD_F_SNDLASTPG);
	    else
		audio1_regs->fifointctl &= ~OUT_HALF_INT_ENA;
	    UNCRIT(j);
	}
	break;

    case AUDIO_RESUME:
	i = *arg;
	if (i & AUDIO_RECEIVE) {
	    j = CRIT();
	    audio_des->flags &= ~AUD_F_RCVPAUSE;
	    if (audio_des->rcv_bufsize > 0) {
		audio_rcv_on(audio_des);
		if (!audio_des->audio2flag && audio_des->flags & AUD_F_16BIT)
		    sync_receive_buffer(audio_des);
	    }
	    UNCRIT(j);
	}

	if (i & AUDIO_TRANSMIT) {
	    j = CRIT();
	    audio_des->flags &= ~AUD_F_SNDPAUSE;
	    if (audio_des->snd_bufcount > 0) {
		if (audio_des->audio2flag) {
		    audio_des->flags |= AUD_F_SNDON;
		    audio2_regs->dstatus |= DSTAT_IE;
		}
		else
		    audio1_regs->fifointctl |= OUT_HALF_INT_ENA;
	    }
	    UNCRIT(j);
	}
	break;

    case AUDIO_DRAIN:
	audio_output_drain(audio_des,PZERO+1);
	break;

    case AUDIO_RAW_SET_PARAMS:
	if (audio_des->audio2flag) {
	    if (audio_des->rcv_bufcount != 0 || audio_des->snd_bufcount != 0)
		return(EBUSY);

	    if (audio2_regs->dstatus & (DSTAT_PC|DSTAT_RC) )
		return(EBUSY);

	    new2_config_ptr = &(((struct raw_audio_config *)arg)->audio_conf_union.cs4215_conf);

	    i = CRIT();
	    audio2_set_config(audio_des,new2_config_ptr->control);

	    audio2_regs->gainctl = new2_config_ptr->gainctl;
	    audio_des->cur_gainctl = new2_config_ptr->gainctl;
	    audio2_regs->ovrange = new2_config_ptr->over_range;
	    audio2_regs->pio = new2_config_ptr->pio;
	    UNCRIT(i);
	}
	else {
	    new1_config_ptr = &(((struct raw_audio_config *)arg)->audio_conf_union.psb2160_conf);
	    audio1_set_config(audio_des,new1_config_ptr);
	}
	break;

    case AUDIO_RAW_GET_PARAMS:
	if (audio_des->audio2flag) {
	    new2_config_ptr = &(((struct raw_audio_config *)arg)->audio_conf_union.cs4215_conf);
	    new2_config_ptr->control    = audio2_regs->cntl;
	    new2_config_ptr->dmastatus  = audio2_regs->dstatus;
	    new2_config_ptr->gainctl    = audio2_regs->gainctl;
	    new2_config_ptr->over_range = audio2_regs->ovrange;
	    new2_config_ptr->pio        = audio2_regs->pio;
	}
	else {
	    new1_config_ptr = &(((struct raw_audio_config *)arg)->audio_conf_union.psb2160_conf);
	    *new1_config_ptr = audio_des->cur_config; /*struct assignment */
	}
	break;

    case AUDIO_METER:
	adiptr = (struct audio_datainfo *)arg;

	adiptr->receive_meter_state = audio_des->receive_meter_state;
	adiptr->transmit_meter_state = audio_des->transmit_meter_state;

	j = CRIT();

#ifdef __hp9000s700

	if (audio_des->audio2flag) {

	    /* Get Partial Page counts */

	    meter_offset = audio2_regs->pcuradd;
	    if (   audio_des->transmit_meter_state == METER_ON
		&& audio_des->scurpage_cnt > 0
		&& ltor(KERNELSPACE,(unsigned long)audio_des->snd_meter_ptr & ~PGOFSET)
		    == (meter_offset & ~PGOFSET) ) {

		/* Align to cache line boundary */

		bufptr = (unsigned char *)( ((unsigned long)audio_des->snd_meter_ptr & ~PGOFSET)
					   | (meter_offset & (PGOFSET & ~(MAX_CACHE_LINE - 1) ) ) );

		i = bufptr - audio_des->snd_meter_ptr;
		if (i > 0)
		    audio2_meter_sum(audio_des,TRUE,FALSE,i);
	    }

	    meter_offset = audio2_regs->rcuradd;
	    if (   audio_des->receive_meter_state == METER_ON
		&& ltor(KERNELSPACE,(unsigned long)audio_des->rcv_meter_ptr & ~PGOFSET)
		   == (meter_offset & ~PGOFSET) ) {

		/* Align to cache line boundary */

		bufptr = (unsigned char *)(  ((unsigned long)audio_des->rcv_meter_ptr & ~PGOFSET)
					   | (meter_offset & (PGOFSET & ~(MAX_CACHE_LINE - 1) ) ) );

		i = bufptr - audio_des->rcv_meter_ptr;
		if (i > 0)
		    audio2_meter_sum(audio_des,FALSE,FALSE,i);
	    }
	}
#endif

	if (audio_des->audio2flag && audio2_regs->ovrange) {

	    /*
	     * We only execute this code if the over range
	     * register is set. We do this so that we only
	     * clear the register if it is set. Otherwise,
	     * we could wind up missing an overrange indication,
	     * since it could be set between the time we test it
	     * and clear it.
	     */

	    adiptr->receive_overrange_flag = audio2_regs->ovrange;
	    audio2_regs->ovrange = 0;
	}
	else
	    adiptr->receive_overrange_flag = 0;

	adiptr->receive_nsamples = audio_des->receive_nsamples;
	adiptr->receive_sample_sum[0] = audio_des->receive_sample_sum[0];
	adiptr->receive_sample_sum[1] = audio_des->receive_sample_sum[1];
	adiptr->receive_sample_peak[0] = audio_des->receive_sample_peak[0];
	adiptr->receive_sample_peak[1] = audio_des->receive_sample_peak[1];

	adiptr->transmit_nsamples = audio_des->transmit_nsamples;
	adiptr->transmit_sample_sum[0] = audio_des->transmit_sample_sum[0];
	adiptr->transmit_sample_sum[1] = audio_des->transmit_sample_sum[1];
	adiptr->transmit_sample_peak[0] = audio_des->transmit_sample_peak[0];
	adiptr->transmit_sample_peak[1] = audio_des->transmit_sample_peak[1];

	audio_des->receive_nsamples = 0;
	audio_des->receive_sample_sum[0] = 0;
	audio_des->receive_sample_sum[1] = 0;
	audio_des->receive_sample_peak[0] = 0;
	audio_des->receive_sample_peak[1] = 0;
	audio_des->transmit_nsamples = 0;
	audio_des->transmit_sample_sum[0] = 0;
	audio_des->transmit_sample_sum[1] = 0;
	audio_des->transmit_sample_peak[0] = 0;
	audio_des->transmit_sample_peak[1] = 0;
	UNCRIT(j);
	break;

    case AUDIO_METER_CTL:
	amcptr = (struct audio_meter_ctl *)arg;

	j = CRIT();
	if (amcptr->receive_meter_state != audio_des->receive_meter_state) {
	    if (amcptr->receive_meter_state == METER_OFF) {
		audio_des->receive_nsamples = 0;
		audio_des->receive_sample_sum[0] = 0;
		audio_des->receive_sample_sum[1] = 0;
		audio_des->receive_sample_peak[0] = 0;
		audio_des->receive_sample_peak[1] = 0;
		if (!audio_des->audio2flag) {
		    if ((audio_des->flags & AUD_F_RCVON) == 0) {
			audio1_regs->interrupt &= ~INFIFO_ENA;
			audio_des->flags &= ~AUD_F_ODD;
			audio_des->receive_sample_save = 0;
		    }
		}
	    }
	    else {
		if (amcptr->receive_meter_state != METER_ON) {
		    UNCRIT(j);
		    return(EINVAL);
		}

		if (audio_des->audio2flag) {
		    if ((audio2_regs->dstatus & DSTAT_IE) == 0) {
			audio2_regs->dstatus |= DSTAT_IE;

			/*
			 * If we are just enabling interrupts, we don't
			 * want to sum the first two "received" pages
			 * in the isr, because they were not really
			 * received via dma input.
			 */

			audio_des->flags |= AUD_F_MTRNOSUM;
		    }

		    if (audio_des->rcurpage_cnt == NBPG)
			audio_des->rcv_meter_ptr = audio_des->rcv_buftail;
		    else
			audio_des->rcv_meter_ptr = audio_des->rcv_extra_ptr;
		}
		else {
		    if ((audio_des->flags & AUD_F_RCVON) == 0)
			audio1_regs->interrupt |= INFIFO_ENA;
		}
	    }

	    audio_des->receive_meter_state = amcptr->receive_meter_state;
	    if (audio_des->flags & AUD_F_MTRNOSUM) {

		/*
		 * We are just starting up inbound dma, so drop
		 * the spl level to normal, which will immediately
		 * cause the isr to be called twice, to "prime" the
		 * pump. We have set AUD_F_MTRNOSUM so that we will
		 * not sum the first two pages, since they were not
		 * really received. We then immediately raise the spl
		 * level and clear the AUD_F_MTRNOSUM flag.
		 */

		UNCRIT(j);

		/* Let isr be called */

		j = CRIT();
		audio_des->flags &= ~AUD_F_MTRNOSUM;
	    }
	}

	if (amcptr->transmit_meter_state != audio_des->transmit_meter_state) {
	    if (amcptr->transmit_meter_state == METER_OFF) {
		audio_des->transmit_nsamples = 0;
		audio_des->transmit_sample_sum[0] = 0;
		audio_des->transmit_sample_sum[1] = 0;
		audio_des->transmit_sample_peak[0] = 0;
		audio_des->transmit_sample_peak[1] = 0;
	    }
	    else {
		if (amcptr->transmit_meter_state != METER_ON) {
		    UNCRIT(j);
		    return(EINVAL);
		}
		if (audio_des->audio2flag)
		    audio_des->snd_meter_ptr = audio_des->snd_bufhead;
	    }
	    audio_des->transmit_meter_state = amcptr->transmit_meter_state;
	}
	UNCRIT(j);
	break;

    case AUDIO_SET_BEEP:
	abtptr = (struct audio_beep_type *)arg;
	i = audio_set_beep(audio_des,abtptr);
	if (i != 0)
	    return(i);
	break;

    case AUDIO_GET_BEEP:
	abtptr = (struct audio_beep_type *)arg;
	if (audio_des->flags & AUD_F_SOUNDBEEP) {
	    abtptr->type = AUDIO_BEEP_ULAW;
	    abtptr->repeat_count = 1;
	    abtptr->sample_rate = 8000;
	    abtptr->nchannels = 1;

	    i = abtptr->returnlen = audio_des->beep_bufdatalen;
	    if (i <= abtptr->datalen)
		abtptr->datalen = i;
	    else
		i = abtptr->datalen;

	    if (i > 0) {
		i = copyout(audio_des->beep_buffer,abtptr->data,i);
		if (i != 0)
		    return(i);
	    }
	}
	else {
	    abtptr->type = AUDIO_BEEP_TONE;
	    abtptr->datalen = abtptr->returnlen = 0;
	}

	break;

    case AUDIO_TEST_MODE:

	if (!audio_des->audio2flag)
	    return(EOPNOTSUPP);

	if (IS_CTL(dev))
	    return(EACCES);

	i = audio_test_mode(audio_des,(struct audio_test_buffer *)arg);
	if (i != 0)
	    return(i);
	break;

    case AUDIO_TEST_RW_REGISTER:

	if (!audio_des->audio2flag)
	    return(EOPNOTSUPP);

	if ((audio_des->flags & AUD_F_TESTMODE) == 0)
	    return(EPERM);

	i = audio_test_rw_register(audio_des,(struct audio_regio *)arg);
	if (i != 0)
	    return(i);
	break;

    case AUDIO_TEST_RW_BUFFER:

	if (!audio_des->audio2flag)
	    return(EOPNOTSUPP);

	if ((audio_des->flags & AUD_F_TESTMODE) == 0)
	    return(EPERM);

	i = audio_test_rw_buffer(audio_des,(struct audio_bufio *)arg);
	if (i != 0)
	    return(i);
	break;

    case AUDIO_TEST_INT:

	if (!audio_des->audio2flag)
	    return(EOPNOTSUPP);

	if ((audio_des->flags & AUD_F_TESTMODE) == 0)
	    return(EPERM);

	i = *arg;
	j = CRIT();
	while ( (audio2_regs->dstatus & (DSTAT_PN|DSTAT_RN)) == 0) {

	    if (i == 0) {
		UNCRIT(j);
		return(EAGAIN);
	    }
	    else {
		audio2_regs->dstatus |= DSTAT_IE;
		sleep((caddr_t)&audio_des->test_buffer,PZERO+1);
	    }
	}
	UNCRIT(j);
	break;

    case DOBEEP:
	biptr = (struct beep_info *)arg;

	/* Don't Allow beeps on the Control Only device if the R/W */
	/* device is open.                                         */

	if (IS_CTL(dev) && (audio_des->flags & AUD_F_RWOPEN))
	    return(EBUSY);

	audio_beeper(audio_des,
		     biptr->frequency,biptr->volume,biptr->duration,
		     BEEPTYPE_GENERAL);
	break;

    default:

	/* wrong! */

	return(EINVAL);
    }

    return(0);
}

audio_read(dev, uio)
    dev_t dev;
    register struct uio *uio;
{
    register struct audio_descriptor *audio_des;
    register struct audio2_reg *audio2_regs;
    register unsigned char *bufptr;
    register unsigned char *purgeptr;
    register int i;
    register int nbytes;
    register int nread;
    register int error;
    unsigned long page_addr1;
    unsigned long page_addr2;
    unsigned long page_addr3;

    if ((audio_des = audio_get_descriptor(m_selcode(dev))) == 0)
	return(ENODEV);

    if (IS_CTL(dev))
	return(EACCES);

    /* disable any beeping on this device and/or restore config */

    if (audio_des->flags & AUD_F_BEEPCONF)
	beeper_reset(audio_des,FALSE);

    nread = uio->uio_resid;

    /* Make sure nread is valid, according to sample size and # of channels */

    i = audio_des->flags & (AUD_F_16BIT|AUD_F_STEREO);
    if (i != 0) {

	if (i == (AUD_F_16BIT|AUD_F_STEREO)) {

	    /* If both, nread must be divisible by 4 */

	    if (nread & 0x3)
		return(EINVAL);
	}
	else {

	    /* Otherwise, nread must be even */

	    if (nread & 0x1)
		return(EINVAL);
	}
    }

    if (audio_des->rcv_bufsize == 0) {

	i = 2 * nread;
	if (i < audio_des->min_bufsize)
	    i = audio_des->min_bufsize;
	else {
	    if (i > audio_des->max_rcv_bufsize) {
		if (nread > audio_des->max_rcv_bufsize)
		    return(EINVAL);

		i = audio_des->max_rcv_bufsize;
	    }
	    else {

		/* Round to nearest page quantity */

		i = (i + NBPG - 1) & ~(NBPG - 1);
	    }
	}

	/* Allocate receive buffer */

	bufptr = (unsigned char *)sys_memall(i);
	if (bufptr == (unsigned char *)0)
	    return(ENOMEM);

#ifdef __hp9000s700
	if (audio_des->audio2flag) {
	    pdcache(KERNELSPACE,bufptr,i);
	    _SYNC();
	}
#endif

	audio_des->rcv_bufsize = i;
	audio_des->rcv_bufcount = 0;
	audio_des->rcv_buffer  = bufptr;
	audio_des->rcv_bufhead = audio_des->rcv_buftail = bufptr;
    }

    /* nread may not be greater than buffer size */

    if (nread > audio_des->rcv_bufsize)
	return(EINVAL);

    do {
	i = CRIT();
	nbytes = audio_des->rcv_bufcount;

	/* The following check is only necessary for Audio I */

	if ((audio_des->flags & AUD_F_16BIT) && (nbytes & 0x1))
	    nbytes--;

	if (nbytes < nread) {
	    if (nbytes == 0) {

		/* Check for receive pause condition */

		if (audio_des->flags & AUD_F_RCVPAUSE) {
		    UNCRIT(i);

		    /* Indicate receive pause condition */

		    return(EIO);
		}

		/* receive operation may be disabled. enable it */

		audio_rcv_on(audio_des);
		if (!audio_des->audio2flag && audio_des->flags & AUD_F_16BIT)
		    sync_receive_buffer(audio_des);
	    }

	    if (audio_des->flags & AUD_F_RCVPAUSE) {
		UNCRIT(i);
		break;
	    }

	    if (uio->uio_fpflags & (FNDELAY|FNBLOCK)) {
		UNCRIT(i);
		if (nbytes == 0) {
		    if (uio->uio_fpflags & FNDELAY)
			return(0);
		    else
			return(EAGAIN);
		}
		break;
	    }

	    audio_des->flags |= AUD_F_RSLEEP;
	    sleep((caddr_t)&audio_des->rcv_bufcount,PZERO+1);
	}

	UNCRIT(i);
    } while (nbytes < nread);

    if (nbytes < nread)
	nread = nbytes;

    /* Transfer data from receive buffer to user buffer
     *
     *
     * Audio II only
     * -------------
     * Before we update rcv_bufhead (once we update it, audio2_isr will
     * feel free to use the available space), we purge the cache for
     * any full pages that have been freed. We do this now, while we
     * are not under interrupt, so that the receive metering in the
     * isr can look at the incoming data.
     */

    bufptr = audio_des->rcv_bufhead;
    if ((bufptr + nread) > (audio_des->rcv_buffer + audio_des->rcv_bufsize)) {
	i = (audio_des->rcv_buffer - bufptr) + audio_des->rcv_bufsize;
	error = uiomove(bufptr,i,UIO_READ,uio);
	if (error != 0)
	    return(error);

#ifdef __hp9000s700
	if (audio_des->audio2flag) {

	    /* Round down to DMA page boundary */

	    purgeptr = (unsigned char *)((unsigned int)bufptr & ~PGOFSET);
	    pdcache(KERNELSPACE,purgeptr,((bufptr - purgeptr) + i + NBPG - 1) & ~PGOFSET);
	    _SYNC();
	}
#endif

	bufptr = audio_des->rcv_buffer;
    }
    else
	i = 0;

    error = uiomove(bufptr,nread-i,UIO_READ,uio);
    if (error != 0)
	return(error);

    if (!audio_des->audio2flag)
	bufptr += (nread - i);
    else {

	/* Round down to DMA page boundary */

	purgeptr = (unsigned char *)((unsigned int)bufptr & ~PGOFSET);
	bufptr += (nread - i);
	i = ((bufptr - purgeptr) + NBPG - 1) & ~PGOFSET;
#ifdef __hp9000s700
	if (i > 0) {
	    pdcache(KERNELSPACE,purgeptr,i);
	    _SYNC();
	}
#endif
    }

    if (bufptr == (audio_des->rcv_buffer + audio_des->rcv_bufsize))
	bufptr = audio_des->rcv_buffer;

#ifdef __hp9000s700
    if (   audio_des->audio2flag
	&& ((unsigned long)audio_des->rcv_bufhead & ~PGOFSET)
	   != ((unsigned long)bufptr & ~PGOFSET) ) {

	/*
	 * For Audio II, if we have freed up a page, see if we are
	 * currently receiving the last page. If so, we might be able
	 * to swap in the newly freed page just in the nick of time.
	 */

	i = CRIT();
	if (audio_des->flags & AUD_F_RCVLASTPG) {

	    audio2_regs = (struct audio2_reg *)audio_des->audio_regs;

	    page_addr1 = ltor(KERNELSPACE,audio_des->rcv_extra_ptr);
	    page_addr2 = ltor(KERNELSPACE,audio_des->rcv_bufhead);

	    /*
	     * We must be able to do the following tests and
	     * substitution if the current page offset is less than
	     * AUDIO_SWAP_THRESHOLD sample bytes. Actually, this should
	     * be a function of the sample rate and CPU speed, but instead
	     * we are just conservative, by going with the most
	     * demanding sampling rate (48K), using a 720 and then
	     * adding some slop.
	     */

	    page_addr3 = audio2_regs->rcuradd;

	    if (   page_addr1 == (page_addr3 & ~PGOFSET)
		&& (page_addr3 & PGOFSET) < AUDIO_SWAP_THRESHOLD) {

		/* Quick! put a new data page in place */

		audio2_regs->rnxtadd = page_addr2;

		/* End of AUDIO_SWAP_THRESHOLD time */

		audio_des->flags &= ~AUD_F_RCVLASTPG;
		audio_des->rnxtpage_cnt = NBPG;
	    }
	}
	UNCRIT(i);
    }
#endif

    /* Update buffer head pointer and count */

    i = CRIT();
    audio_des->rcv_bufhead = bufptr;
    audio_des->rcv_bufcount -= nread;
    UNCRIT(i);

    return(0);
}

audio_write(dev, uio)
    dev_t dev;
    struct uio *uio;
{
    register struct audio_descriptor *audio_des;
    register struct audio1_reg *audio1_regs;
    register struct audio2_reg *audio2_regs;
    register unsigned char *bufptr;
    register int i;
    register int j;
    register int nbytes;
    register int nwrite;
    unsigned char *orig_tailptr;
    unsigned char *data_ptr;
    unsigned long page_addr1;
    unsigned long page_addr2;
    unsigned long page_addr3;

    if ((audio_des = audio_get_descriptor(m_selcode(dev))) == 0)
	return(ENODEV);

    if (IS_CTL(dev))
	return(EACCES);

    /* disable any beeping on this device and/or restore config */

    if (audio_des->flags & AUD_F_BEEPCONF)
	beeper_reset(audio_des,FALSE);

    nwrite = uio->uio_resid;

    /* Make sure nwrite is valid, according to sample size and # of channels */

    i = audio_des->flags & (AUD_F_16BIT|AUD_F_STEREO);
    if (i != 0) {

	if (i == (AUD_F_16BIT|AUD_F_STEREO)) {

	    /* If both, nwrite must be divisible by 4 */

	    if (nwrite & 0x3)
		return(EINVAL);
	}
	else {

	    /* Otherwise, nwrite must be even */

	    if (nwrite & 0x1)
		return(EINVAL);
	}
    }

    if (audio_des->snd_bufsize == 0) {

	i = 2 * nwrite;
	if (i < audio_des->min_bufsize)
	    i = audio_des->min_bufsize;
	else {
	    if (i > audio_des->max_snd_bufsize) {
		if (nwrite > audio_des->max_snd_bufsize)
		    return(EINVAL);

		i = audio_des->max_snd_bufsize;
	    }
	    else {

		/* Round to nearest page quantity */

		i = (i + NBPG - 1) & ~(NBPG - 1);
	    }
	}

	/* Allocate send buffer */

	bufptr = (unsigned char *)sys_memall(i);
	if (bufptr == (unsigned char *)0)
	    return(ENOMEM);

	audio_des->snd_bufsize = i;
	audio_des->snd_bufcount = 0;
	audio_des->snd_buffer  = bufptr;
	audio_des->snd_bufhead = audio_des->snd_buftail = bufptr;
	audio_des->snd_meter_ptr = bufptr;
    }

    /* nwrite may not be greater than buffer size */

    j = audio_des->snd_bufsize;
    if (nwrite > j)
	return(EINVAL);

    i = CRIT();

    do {
	nbytes = j - audio_des->snd_bufcount;

	if (nbytes < nwrite) {

	    /* Return an error if we are paused */

	    if (audio_des->flags & AUD_F_SNDPAUSE) {
		UNCRIT(i);
		return(EIO);
	    }

	    audio_des->flags |= AUD_F_WSLEEP;
	    sleep((caddr_t)&audio_des->snd_bufcount,PZERO+1);
	}
    } while (nbytes < nwrite);

    bufptr = audio_des->snd_buftail;
    if (audio_des->audio2flag)
	orig_tailptr = bufptr;

    UNCRIT(i);

    /* Transfer data from user buffer to send buffer */

    if ((bufptr + nwrite) > (audio_des->snd_buffer + audio_des->snd_bufsize)) {
	i = (audio_des->snd_buffer - bufptr) + audio_des->snd_bufsize;
	j = uiomove(bufptr,i,UIO_WRITE,uio);
	if (j != 0)
	    return(j);

#ifdef __hp9000s700
	if (audio_des->audio2flag) {
	    fdcache(KERNELSPACE,bufptr,i);
	    _SYNC();
	}
#endif

	bufptr = audio_des->snd_buffer;
    }
    else
	i = 0;

    j = uiomove(bufptr,nwrite-i,UIO_WRITE,uio);
    if (j != 0)
	return(j);

    if (!audio_des->audio2flag)
	bufptr += (nwrite - i);
    else {
#ifdef __hp9000s700

	fdcache(KERNELSPACE,bufptr,nwrite - i);
	_SYNC();

	bufptr += (nwrite - i);

	/*
	 * If bufptr is not on a page boundary, we need to fill
	 * the rest of the page with silence, In case this is the
	 * last page. Hopefully, most applications will do writes
	 * in multiples of the page size, so we don't have to do
	 * this.
	 */

	i = (unsigned int)bufptr & PGOFSET;
	if (i)
	    silence_fill(audio_des,bufptr,NBPG-i);
#endif
    }

    if (bufptr == (audio_des->snd_buffer + audio_des->snd_bufsize))
	bufptr = audio_des->snd_buffer;

    i = CRIT();

#ifdef __hp9000s700
    if (audio_des->audio2flag) {

	/*
	 * Check to see if isr modified the tail pointer.
	 * If it did, we need to determine which tail pointer
	 * to use.
	 */

	if (audio_des->snd_buftail != orig_tailptr) {
	    if (    ((unsigned int)bufptr & ~PGOFSET)
		 == ((unsigned int)orig_tailptr & ~PGOFSET)  ) {

		/* Use ISR version */

		bufptr = audio_des->snd_buftail;
		nwrite = 0;
	    }
	    else {

		/* Use our version */

		nwrite -= (NBPG - ((unsigned int)orig_tailptr & PGOFSET));
	    }
	}

	if (nwrite != 0 && (audio_des->flags & AUD_F_SNDLASTPG)) {

	    /* Check to see if we can make the substitution */

	    audio2_regs = (struct audio2_reg *)audio_des->audio_regs;
	    page_addr1 = ltor(KERNELSPACE,audio_des->snd_extra_ptr);
	    page_addr2 = ltor(KERNELSPACE,audio_des->snd_buftail);
	    page_addr3 = audio2_regs->pcuradd;

	    if (page_addr1 == (page_addr3 & ~PGOFSET)
		&& (page_addr3 & PGOFSET) < AUDIO_SWAP_THRESHOLD ) {

		audio2_regs->pnxtadd = page_addr2;

		audio_des->flags &= ~AUD_F_SNDLASTPG;

		if (nwrite >= NBPG)
		    audio_des->snxtpage_cnt = NBPG;
		else {
		    audio_des->snxtpage_cnt = nwrite;

		    /*
		     * We are pushing out a partial page, pretend
		     * it's full.
		     */

		    j = (unsigned int)bufptr & PGOFSET;
		    if (j > 0) {
			nwrite = NBPG;
			bufptr += NBPG - j;
			if (bufptr == (audio_des->snd_buffer + audio_des->snd_bufsize))
			    bufptr = audio_des->snd_buffer;
		    }
		}
	    }
	}
    }
#endif

    /* Update buffer head pointer and count, and  */
    /* enable output fifo half empty interrupt    */

    audio_des->snd_buftail = bufptr;
    audio_des->snd_bufcount += nwrite;
    if ((audio_des->flags & AUD_F_SNDPAUSE) == 0) {
	if (audio_des->audio2flag) {
	    audio_des->flags |= AUD_F_SNDON;
	    audio2_regs = (struct audio2_reg *)audio_des->audio_regs;
	    audio2_regs->dstatus |= DSTAT_IE;
	}
	else {
	    audio1_regs = (struct audio1_reg *)audio_des->audio_regs;
	    audio1_regs->fifointctl |= OUT_HALF_INT_ENA;
	}
    }
    UNCRIT(i);

    return(0);
}

audio_select(dev, which)
    dev_t dev;
    int which;
{
    register struct audio_descriptor *audio_des;
    register struct proc *p;
    register int i;

    if ((audio_des = audio_get_descriptor(m_selcode(dev))) == 0)
	return(1);

    switch (which) {

    case FREAD:
	i = CRIT();
	if (   audio_des->rcv_bufsize == 0
	    || audio_des->flags & AUD_F_RCVPAUSE
	    || audio_des->rcv_bufcount >= audio_des->rcv_sel_threshold ) {

	    UNCRIT(i);
	    return(1);
	}

	if ((p = audio_des->audio_rsel) && (p->p_wchan == (caddr_t) &selwait))
	    audio_des->flags |= AUD_F_RCOLL;
	else
	    audio_des->audio_rsel = u.u_procp;

	/* If bufcount is zero, we should enable input */

	if (audio_des->rcv_bufcount == 0) {
	    audio_rcv_on(audio_des);
	    if (!audio_des->audio2flag && audio_des->flags & AUD_F_16BIT)
		sync_receive_buffer(audio_des);
	}
	UNCRIT(i);
	break;

    case FWRITE:
	i = CRIT();
	if (   audio_des->snd_bufsize == 0
	    || (audio_des->snd_bufsize - audio_des->snd_bufcount)
	       >= audio_des->snd_sel_threshold ) {

	    UNCRIT(i);
	    return(1);
	}

	if ((p = audio_des->audio_wsel) && (p->p_wchan == (caddr_t) &selwait))
	    audio_des->flags |= AUD_F_WCOLL;
	else
	    audio_des->audio_wsel = u.u_procp;

	UNCRIT(i);
	break;
    }

    return(0);
}

/* There may be a problem when using 16 bit format, since it needs   */
/* to be synced. Since we are constantly reading the fifo even when  */
/* we are not loading the data into the receive buffer, we could be  */
/* out of sync when we start loading the receive buffer again. The   */
/* audio isr keeps track of whether the current data byte is odd or  */
/* even (most significant byte or least significant byte).           */

void
sync_receive_buffer(audio_des)
    register struct audio_descriptor *audio_des;
{
    /* This algorithm relies on the fact that the receive buffer */
    /* is aligned on an even boundary and that the buffer size   */
    /* is an even number of bytes long. We don't need to worry   */
    /* about queue wraparound.                                   */

    if ( ( (int)(audio_des->rcv_buftail) ) & 0x1) {
	if ((audio_des->flags & AUD_F_ODD) == 0)  {

	    audio_des->rcv_buftail--;
	    audio_des->rcv_bufcount--;
	}
    }
    else {
	if (audio_des->flags & AUD_F_ODD) {
	    *audio_des->rcv_buftail++ = audio_des->receive_sample_save;
	    audio_des->rcv_bufcount++;
	}
    }

    return;
}

/*
 * interrupt service routine for Audio I
 */

#ifdef __hp9000s700
void
audio1_isr(sc,audio_des)
    struct isc_table_type *sc;
    struct audio_descriptor *audio_des;
#else
void
audio1_isr(inf)
    struct interrupt *inf;
#endif
{
    register int int_cond;
    register int i;
    register int rcv_bufsize;
    register int prev_data;
    register int cur_data;
    register int record_sample_flag;
#ifdef __hp9000s300
    register struct audio_descriptor *audio_des
	     = (struct audio_descriptor *)inf->temp;
#endif
    register struct audio1_reg *audio1_regs;
    register unsigned char *bufptr;
    register unsigned char *bufend;
    unsigned short *conv_tab;

    audio1_regs = (struct audio1_reg *)audio_des->audio_regs;
    int_cond = audio1_regs->fifointctl;

    /* On Series 700, it is necessary to disable the interrupts, since */
    /* we can have multiple interrupt "Causes". If we fix cause A,     */
    /* and then while fixing cause B, A occurs again, the interrupt    */
    /* request will not be dropped. On Series 700 it is mandatory for  */
    /* the interrupt to be dropped before it will be redelivered, so   */
    /* we make sure that will happen by disabling interrupts and then  */
    /* re-enabling them at the end of the isr.                         */

    audio1_regs->interrupt &= ~INT_ENA;

    if (int_cond & IN_HALF_INT_REQ) {

	/*
	 * If the input fifo is full or there is no room on the
	 * receive queue, then set the receive pause condition,
	 * disable receive operation, and increment the overflow count.
	 *
	 * NOTE: We use the local variable "rcv_bufsize" for two
	 * purposes. Since we reference the buffer size fairly often,
	 * we store the value in a register variable for quicker access.
	 * We also use the variable as a flag, by setting the variable
	 * to zero. A value of zero can mean that either we really do
	 * do not have an allocated receive buffer, or that the buffer
	 * is full.
	 */

	bufptr = audio_des->rcv_buftail;
	i = audio_des->rcv_bufcount;
	rcv_bufsize = audio_des->rcv_bufsize;
	if (audio_des->flags & AUD_F_RCVON) {
	    if  ( (i == rcv_bufsize) || (audio1_regs->fifostatus & IN_FULL) ) {

		audio_des->flags |= AUD_F_RCVPAUSE;
		audio_rcv_off(audio_des);
		audio_des->rcv_overflow_cnt++;
		rcv_bufsize = 0;
	    }
	    else
		bufend = audio_des->rcv_buffer + rcv_bufsize;
	}
	else
	    rcv_bufsize = 0;

	if (audio_des->flags & AUD_F_16BIT) {
	    if (audio_des->flags & AUD_F_ODD) {
		prev_data = audio_des->receive_sample_save;
		record_sample_flag = TRUE;
	    }
	    else
		record_sample_flag = FALSE;
	}
	else {
	    record_sample_flag = TRUE;
	    if (audio_des->audio_format == AUDIO_FORMAT_ULAW)
		conv_tab = ulaw_tab;
	    else
		conv_tab = alaw_tab;
	}

	while ((audio1_regs->fifostatus & IN_EMPTY) == 0) {

	    /* Get data */

	    cur_data = audio1_regs->inoutfifo;

	    if (rcv_bufsize != 0) {
		if (i < rcv_bufsize) {

		    /* Move data */

		    *bufptr++ = cur_data;
		    if (bufptr == bufend)
			bufptr = audio_des->rcv_buffer;
		    i++;
		}
		else {
		    audio_des->flags |= AUD_F_RCVPAUSE;
		    audio_rcv_off(audio_des);
		    audio_des->rcv_overflow_cnt++;
		    rcv_bufsize = 0;
		}
	    }

	    /*
	     * Support for AUDIO_METER ioctl
	     *
	     * Note: record_sample_flag is always true
	     *       if we are using an 8 bit format.
	     */

	    if (!record_sample_flag) {
		prev_data = cur_data;
		record_sample_flag = TRUE;
	    }
	    else {
		if (audio_des->receive_meter_state == METER_OFF) {
		    if (audio_des->flags & AUD_F_16BIT)
			record_sample_flag = FALSE;
		}
		else {
		    if (audio_des->flags & AUD_F_16BIT) {
			cur_data |= (prev_data << 8);
			if (cur_data > 32767)
			    cur_data = 65536 - cur_data; /* absolute value */

			record_sample_flag = FALSE;
		    }
		    else
			cur_data = conv_tab[cur_data & 0x7f];

		    audio_des->receive_nsamples++;
		    audio_des->receive_sample_sum[0] += cur_data;
		    if (cur_data > audio_des->receive_sample_peak[0])
			audio_des->receive_sample_peak[0] = cur_data;
		}
	    }
	}

	audio_des->rcv_bufcount = i;
	audio_des->rcv_buftail = bufptr;

	if (audio_des->flags & AUD_F_16BIT) {
	    if (record_sample_flag == FALSE)
		audio_des->flags &= ~AUD_F_ODD;
	    else {
		audio_des->flags |= AUD_F_ODD;
		audio_des->receive_sample_save = prev_data;
	    }
	}

	/* Do read wakeup */

	if (audio_des->flags & AUD_F_RSLEEP) {
	    audio_des->flags &= ~AUD_F_RSLEEP;
	    wakeup((caddr_t)&audio_des->rcv_bufcount);
	}

	/* Do select wakeups */

	if (   audio_des->audio_rsel
	    && (   audio_des->flags & AUD_F_RCVPAUSE
		|| audio_des->rcv_bufcount >= audio_des->rcv_sel_threshold)) {

	    selwakeup(audio_des->audio_rsel, audio_des->flags & AUD_F_RCOLL);
	    audio_des->flags &= ~AUD_F_RCOLL;
	    audio_des->audio_rsel = (struct proc *)0;
	}
    }

    if (int_cond & OUT_HALF_INT_REQ) {

	if ((audio_des->flags & (AUD_F_BEEP|AUD_F_SOUNDBEEP))
	    == (AUD_F_BEEP|AUD_F_SOUNDBEEP)) {

	    /*
	     * We currently have a digitized sample for our beep.
	     * transfer the data from the beep buffer while
	     * there is room in the fifo.
	     */

	    if ((i = audio_des->beep_bufcount) == 0)
		beeper_off(audio_des);
	    else {
		bufptr = audio_des->beep_bufptr;
		while ((audio1_regs->fifostatus & OUT_FULL) == 0 && i > 0) {

		    /* Move data */

		    audio1_regs->inoutfifo = *bufptr++;
		    i--;
		}
		audio_des->beep_bufcount = i;
		audio_des->beep_bufptr = bufptr;
	    }
	}
	else {

	    /*
	     * Disable half empty ouput fifo interrupt
	     * if there is nothing to send.
	     * Also increment underflow count if not draining.
	     */

	    if ((i = audio_des->snd_bufcount) == 0) {
		audio1_regs->fifointctl &= ~OUT_HALF_INT_ENA;
		if ((audio1_regs->fifointctl & OUT_EMPTY_INT_ENA) == 0)
		    audio_des->snd_underflow_cnt++;
	    }
	    else {
		bufptr = audio_des->snd_bufhead;
		bufend = audio_des->snd_buffer + audio_des->snd_bufsize;

		if (audio_des->transmit_meter_state == METER_ON) {
		    if (audio_des->audio_format == AUDIO_FORMAT_ULAW)
			conv_tab = ulaw_tab;
		    else
			conv_tab = alaw_tab;
		}

		while ((audio1_regs->fifostatus & OUT_FULL) == 0 && i > 0) {

		    if (audio_des->transmit_meter_state == METER_ON) {
			if (audio_des->flags & AUD_F_16BIT) {

			    /*
			     * We rely on the fact that the buffer
			     * starts on an even word boundary, and
			     * that we don't allow odd writes when in
			     * 16 bit mode.
			     */

			    if ( ((int)bufptr) & 0x1)
				cur_data = -1;
			    else {
				cur_data = *((short *)bufptr);
				if (cur_data < 0)
				    cur_data = -cur_data;
			    }
			}
			else {
			    cur_data = conv_tab[*bufptr & 0x7f];
			}

			if (cur_data != -1) {
			    audio_des->transmit_nsamples++;
			    audio_des->transmit_sample_sum[0] += cur_data;
			    if (cur_data > audio_des->transmit_sample_peak[0])
				audio_des->transmit_sample_peak[0] = cur_data;
			}
		    }

		    /* Move data */

		    audio1_regs->inoutfifo = *bufptr++;
		    if (bufptr == bufend)
			bufptr = audio_des->snd_buffer;
		    i--;
		}
		audio_des->snd_bufcount = i;
		audio_des->snd_bufhead = bufptr;

		/* Do write wakeup */

		if (audio_des->flags & AUD_F_WSLEEP) {
		    audio_des->flags &= ~AUD_F_WSLEEP;
		    wakeup((caddr_t)&audio_des->snd_bufcount);
		}

		/* Do select wakeups */

		if (   audio_des->audio_wsel
		    && (audio_des->snd_bufsize - audio_des->snd_bufcount)
		       >= audio_des->snd_sel_threshold ) {

		    selwakeup(audio_des->audio_wsel, audio_des->flags & AUD_F_WCOLL);
		    audio_des->flags &= ~AUD_F_WCOLL;
		    audio_des->audio_wsel = (struct proc *)0;
		}
	    }
	}
    }

    if (int_cond & OUT_EMPTY_INT_REQ) {
	wakeup((caddr_t)&audio1_regs->inoutfifo);
	audio1_regs->fifointctl &= ~OUT_EMPTY_INT_ENA;
    }

    /* Re-enable interrupts */

    audio1_regs->interrupt |= INT_ENA;

    return;
}

/*
 * interrupt service routine for Audio II
 */

#ifdef __hp9000s700
void
audio2_isr(sc,audio_des)
    struct isc_table_type *sc;
    struct audio_descriptor *audio_des;
{
    register int int_cond;
    register int i;
    register int remain_cnt;
    register unsigned char *data_ptr;
    register struct audio2_reg *audio2_regs;

    audio2_regs = (struct audio2_reg *)audio_des->audio_regs;
    int_cond = audio2_regs->dstatus;

    /* On Series 700, it is necessary to disable the interrupts, since */
    /* we can have multiple interrupt "Causes". If we fix cause A,     */
    /* and then while fixing cause B, A occurs again, the interrupt    */
    /* request will not be dropped. On Series 700 it is mandatory for  */
    /* the interrupt to be dropped before it will be redelivered, so   */
    /* we make sure that will happen by disabling interrupts and then  */
    /* re-enabling them at the end of the isr.                         */

    audio2_regs->dstatus &= ~DSTAT_IE;

    if (audio_des->flags & AUD_F_TESTMODE) {
	wakeup((caddr_t)&audio_des->test_buffer);
	return;
    }

    if (int_cond & DSTAT_PN) {

	/*
	 * If the page that just finished had audio buffer data,
	 * we can now make the space available for writing.
	 */

	if (audio_des->scurpage_cnt > 0) {

	    if (audio_des->scurpage_cnt != NBPG)
		panic("Bad audio send buffer current page count");

	    if (audio_des->transmit_meter_state == METER_ON) {
		i = audio_des->snd_meter_ptr - audio_des->snd_bufhead;
		audio2_meter_sum(audio_des,TRUE,TRUE,NBPG - i);
	    }

	    audio_des->snd_bufcount -= NBPG;
	    audio_des->snd_bufhead += NBPG;
	    if (audio_des->snd_bufhead == audio_des->snd_buffer + audio_des->snd_bufsize)
		audio_des->snd_bufhead = audio_des->snd_buffer;

	    /* Do write wakeup */

	    if (audio_des->flags & AUD_F_WSLEEP) {
		audio_des->flags &= ~AUD_F_WSLEEP;
		wakeup((caddr_t)&audio_des->snd_bufcount);
	    }

	    /* Do select wakeups */

	    if (   audio_des->audio_wsel
		&& (audio_des->snd_bufsize - audio_des->snd_bufcount)
		   >= audio_des->snd_sel_threshold ) {

		selwakeup(audio_des->audio_wsel, audio_des->flags & AUD_F_WCOLL);
		audio_des->flags &= ~AUD_F_WCOLL;
		audio_des->audio_wsel = (struct proc *)0;
	    }
	}


	if (audio_des->snxtpage_cnt & PGOFSET) {

	    /*
	     * If we pushed a partial page out it is considered
	     * an underflow unless AUD_F_SNDFLUSH is set.
	     * We delay this accounting until now so that
	     * the user has a chance to do an output drain after
	     * the last write.
	     */

	    audio_des->scurpage_cnt = NBPG;
	    if ((audio_des->flags & AUD_F_SNDFLUSH) == 0)
		audio_des->snd_underflow_cnt++;
	}
	else
	    audio_des->scurpage_cnt = audio_des->snxtpage_cnt;

	/*
	 * If snxtpage_cnt is still 0 at bottom of this
	 * then send a page from the extra buffer  (possibly
	 * filled with beep data).
	 */

	audio_des->snxtpage_cnt = 0;

	remain_cnt = audio_des->snd_bufcount - audio_des->scurpage_cnt;

	if ((audio_des->flags & (AUD_F_SNDON|AUD_F_SNDPAUSE)) == AUD_F_SNDON) {

	    if (audio_des->flags & AUD_F_SNDLASTPG) {

		/*
		 * Is this a "clean" shutoff? If so, don't
		 * increment the underflow count. Note that
		 * AUD_F_SNDLASTPG is only set if we sent a
		 * temporary silent buffer. If we forced a
		 * partial buffer out, we have already done
		 * the underflow accounting.
		 */

		if ((audio_des->flags & AUD_F_SNDFLUSH) == 0)
		    audio_des->snd_underflow_cnt++;

		audio_des->flags &= ~(AUD_F_SNDLASTPG|AUD_F_SNDON);
	    }

	    if (remain_cnt == 0) {

		/*
		 * We will give the users one last chance
		 * to mend their ways. If they can feed
		 * us enough data, before the hardware
		 * actually uses the temp buffer, then we can
		 * reset the AUD_F_SNDLASTPG flag.
		 * Note that if remain count is > 0 and < NBPG
		 * we force the page out, since we don't know
		 * if anything else is coming.
		 * Note also, due to these semantics, the only
		 * workable write size for an 8K buffer size (the
		 * minimum) is 4K. Once the buffer size is 12K or
		 * larger, smaller writes may work.
		 */

		audio_des->flags |= AUD_F_SNDLASTPG;
	    }
	    else {
		data_ptr = audio_des->snd_bufhead + audio_des->scurpage_cnt;
		if (data_ptr == audio_des->snd_buffer + audio_des->snd_bufsize)
		    data_ptr = audio_des->snd_buffer;

		audio2_regs->pnxtadd = ltor(KERNELSPACE,data_ptr);
		if (remain_cnt >= NBPG)
		    audio_des->snxtpage_cnt = NBPG;
		else {

		    audio_des->snxtpage_cnt = remain_cnt;

		    /*
		     * We pretend we sent a full page.
		     */

		    i = NBPG - remain_cnt;
		    audio_des->snd_bufcount += i;
		    audio_des->snd_buftail += i;
		    if (audio_des->snd_buftail
			== audio_des->snd_buffer + audio_des->snd_bufsize) {

			audio_des->snd_buftail = audio_des->snd_buffer;
		    }
		}
	    }
	}

	if (audio_des->snxtpage_cnt == 0) {

	    /*
	     * Are we currently beeping? If so, beep_fill will fill
	     * the send extra buffer with beep data. beep_fill is also
	     * responsible for detecting beep completion and refilling
	     * the send extra buffer with silence data.
	     */

	    if (audio_des->flags & AUD_F_BEEP)
		beep_fill(audio_des);

	    audio2_regs->pnxtadd = ltor(KERNELSPACE,audio_des->snd_extra_ptr);

	    if (audio_des->snd_extra_ptr == audio_des->snd_extra_buffer)
		audio_des->snd_extra_ptr += NBPG;
	    else
		audio_des->snd_extra_ptr = audio_des->snd_extra_buffer;
	}

	audio_des->snd_minus_rcv_cnt++;

	if (   (audio_des->flags & AUD_F_SNDFLUSH)
	    && audio_des->snxtpage_cnt == 0
	    && audio_des->scurpage_cnt == 0 ) {

	    audio_des->flags &= ~AUD_F_SNDFLUSH;
	    wakeup((caddr_t)&audio2_regs->pnxtadd);
	}
    }

    if (int_cond & DSTAT_RN) {

	/*
	 * Determine if a receive data page just finished being loaded.
	 */

	if (audio_des->rcurpage_cnt == NBPG) {

	    if (audio_des->receive_meter_state == METER_ON) {
		i = audio_des->rcv_meter_ptr - audio_des->rcv_buftail;
		audio2_meter_sum(audio_des,FALSE,TRUE,NBPG - i);
	    }

	    /* Advance tail */

	    audio_des->rcv_bufcount += NBPG;
	    audio_des->rcv_buftail += NBPG;
	    if (audio_des->rcv_buftail == audio_des->rcv_buffer + audio_des->rcv_bufsize)
		audio_des->rcv_buftail = audio_des->rcv_buffer;

	    /* Do read wakeup */

	    if (audio_des->flags & AUD_F_RSLEEP) {
		audio_des->flags &= ~AUD_F_RSLEEP;
		wakeup((caddr_t)&audio_des->rcv_bufcount);
	    }

	    /* Do select wakeups */

	    if (   audio_des->audio_rsel
		&& (   audio_des->flags & AUD_F_RCVPAUSE
		    || audio_des->rcv_bufcount >= audio_des->rcv_sel_threshold)) {

		selwakeup(audio_des->audio_rsel, audio_des->flags & AUD_F_RCOLL);
		audio_des->flags &= ~AUD_F_RCOLL;
		audio_des->audio_rsel = (struct proc *)0;
	    }
	}
	else {

	    if (audio_des->receive_meter_state == METER_ON) {
		i = audio_des->rcv_meter_ptr - audio_des->rcv_extra_ptr;
		audio2_meter_sum(audio_des,FALSE,TRUE,NBPG - i);
	    }
	}

	audio_des->rcurpage_cnt = audio_des->rnxtpage_cnt;

	i = audio_des->rcv_bufsize
	    - audio_des->rcv_bufcount
	    - audio_des->rcurpage_cnt;

	if ( (audio_des->flags & (AUD_F_RCVLASTPG|AUD_F_RCVPAUSE|AUD_F_RCVON))
	      == AUD_F_RCVON
	    && i >= NBPG ) {

	    /* Receive the next page of data */

	    data_ptr = audio_des->rcv_buftail + audio_des->rcurpage_cnt;
	    if (data_ptr == audio_des->rcv_buffer + audio_des->rcv_bufsize)
		data_ptr = audio_des->rcv_buffer;


	    audio2_regs->rnxtadd = ltor(KERNELSPACE,data_ptr);
	    audio_des->rnxtpage_cnt = NBPG;
	}
	else {

	    /* Receive into the extra buffer */

	    if (audio_des->receive_meter_state == METER_ON) {
		pdcache(KERNELSPACE,audio_des->rcv_extra_ptr,NBPG);
		_SYNC();
	    }

	    audio2_regs->rnxtadd = ltor(KERNELSPACE,audio_des->rcv_extra_ptr);
	    audio_des->rnxtpage_cnt = 0;

	    /* Toggle extra page */

	    if (audio_des->rcv_extra_ptr == audio_des->rcv_extra_buffer)
		audio_des->rcv_extra_ptr += NBPG;
	    else
		audio_des->rcv_extra_ptr = audio_des->rcv_extra_buffer;

	    if (audio_des->flags & AUD_F_RCVON) {
		if (audio_des->flags & AUD_F_RCVLASTPG) {

		    audio_des->flags |= AUD_F_RCVPAUSE;
		    audio_des->flags &= ~(AUD_F_RCVON|AUD_F_RCVLASTPG);
		    audio_des->rcv_overflow_cnt++;
		}
		else {

		    /*
		     * We will give the users one last chance
		     * to mend their ways. If they can read
		     * enough data (to free up space), before the hardware
		     * actually uses this buffer, then we can
		     * reset the AUD_F_RCVLASTPG flag. Meanwhile
		     * we will give the hardware a page out of the
		     * receive extra buffer, so that metering will
		     * continue to work. If the user reads enough
		     * out of the receive buffer before this page gets
		     * used, we may be able to switch to the newly
		     * available page just in the nick of time.
		     */

		    audio_des->flags |= AUD_F_RCVLASTPG;
		}
	    }
	}

	audio_des->snd_minus_rcv_cnt--;

	if (   (audio_des->flags & AUD_F_RCVDRAIN)
	    && audio_des->rnxtpage_cnt == 0
	    && audio_des->rcurpage_cnt == 0 ) {

	    audio_des->flags &= ~AUD_F_RCVDRAIN;
	    wakeup((caddr_t)&audio2_regs->rnxtadd);
	}
    }

    /*
     * Re-enable interrupts if one of many conditions are met.
     */

    if (   audio_des->snxtpage_cnt != 0
	|| audio_des->rnxtpage_cnt != 0
	|| audio_des->scurpage_cnt != 0
	|| audio_des->rcurpage_cnt != 0
	|| audio_des->receive_meter_state == METER_ON
	|| (audio_des->flags & (AUD_F_BEEP|AUD_F_RCVON|AUD_F_SNDON))
	|| audio_des->snd_minus_rcv_cnt != 0 ) {

	audio2_regs->dstatus |= DSTAT_IE;
    }

    return;
}
#endif

/*
 * linking and initialization routines
 */

#ifdef __hp9000s700
extern int (*core_attach)();
int (*audio_saved_attach)();
#else
extern int (*make_entry)();
int (*audio_saved_make_entry)();
#endif


#ifdef __hp9000s700
int
audio_attach(id, isc)
#else
int
audio_make_entry(id, isc)
#endif
    int id;
    struct isc_table_type *isc;
{
    register struct audio_descriptor *audio_des;
    register struct audio1_reg *audio1_regs;
    register struct audio2_reg *audio2_regs;
    register int i;

    if (   id != AUDIO_1_CARD_ID && id != AUDIO_1_NOBEEPER_CARD_ID
	&& id != AUDIO_2_CARD_ID && id != AUDIO_2_NOBEEPER_CARD_ID)
#ifdef __hp9000s700
	return((*audio_saved_attach)(id, isc));

    MALLOC_ETC(audio_des, struct audio_descriptor *,
                sizeof(struct audio_descriptor), M_DEVBUF, M_WAITOK,
	        "audio_attach(): MALLOC");

    if (isc != (struct isc_table_type *)0)
	isc->int_lvl = AUDIO_INT_LEVEL;
    if (id == AUDIO_1_CARD_ID || id == AUDIO_1_NOBEEPER_CARD_ID)
	audio1_regs  = (struct audio1_reg *) isc->if_reg_ptr;
    else {
	if (isc == (struct isc_table_type *)0)
	    audio2_regs = (struct audio2_regs *) AUDIO_2_HPA;
	else
	    audio2_regs  = (struct audio2_reg *) isc->if_reg_ptr;
    }
#else
	return((*audio_saved_make_entry)(id, isc));

    /* Check for right interrupt level now, so that    */
    /* we don't bother allocating memory if it's wrong */

    if (isc->int_lvl != AUDIO_INT_LEVEL)
	return(io_inform("Digital Audio Interface", isc, AUDIO_INT_LEVEL));

    /* Allocate an audio descriptor */

    audio_des = (struct audio_descriptor *)calloc(sizeof(struct audio_descriptor));
    if (audio_des == (struct audio_descriptor *)0)
	return(io_inform("Digital Audio Interface", isc, -2, " ignored; no more memory"));

    if (id == AUDIO_1_CARD_ID || id == AUDIO_1_NOBEEPER_CARD_ID)
	audio1_regs  = (struct audio1_reg *) (isc->card_ptr);
    else
	audio2_regs  = (struct audio2_reg *) (isc->card_ptr);
#endif

    /* Do Common Initialization */

    /* Initialize audio descriptor */

    audio_des->rcv_bufsize  = 0;
    audio_des->rcv_bufcount = 0;
    audio_des->rcv_buffer   = (unsigned char *)0;
    audio_des->rcv_buftail  = (unsigned char *)0;
    audio_des->rcv_bufhead  = (unsigned char *)0;
    audio_des->snd_bufsize  = 0;
    audio_des->snd_bufcount = 0;
    audio_des->snd_buffer   = (unsigned char *)0;
    audio_des->snd_buftail  = (unsigned char *)0;
    audio_des->snd_bufhead  = (unsigned char *)0;
    audio_des->rcv_sel_threshold = DEF_SEL_THRESHOLD;
    audio_des->snd_sel_threshold = DEF_SEL_THRESHOLD;
    if (isc == (struct isc_table_type *)0)
	audio_des->select_code = audio_probe_sc;
    else
	audio_des->select_code = isc->my_isc;
    audio_des->cur_gain[0].receive_gain = 0;
    audio_des->cur_gain[1].receive_gain = 0;
    audio_des->cur_gain[0].transmit_gain = 0;
    audio_des->cur_gain[1].transmit_gain = 0;
    audio_des->cur_gain[0].monitor_gain = AUDIO_OFF_GAIN;
    audio_des->cur_gain[1].monitor_gain = AUDIO_OFF_GAIN;
    audio_des->audio_output = (AUDIO_OUT_INTERNAL|AUDIO_OUT_EXTERNAL); 
    audio_des->audio_input  = AUDIO_IN_MIKE;
    audio_des->audio_format = AUDIO_FORMAT_ULAW;
    audio_des->audio_sample_rate = 8000;
    audio_des->rcv_overflow_cnt  = 0;
    audio_des->snd_underflow_cnt = 0;
    audio_des->open_count = 0;
    audio_des->audio_rsel = (struct proc *)0;
    audio_des->audio_wsel = (struct proc *)0;
    audio_des->receive_nsamples = 0;
    audio_des->receive_sample_sum[0] = audio_des->receive_sample_sum[1] = 0;
    audio_des->receive_sample_peak[0] = audio_des->receive_sample_peak[1] = 0;
    audio_des->receive_sample_save  = 0;
    audio_des->transmit_nsamples = 0;
    audio_des->transmit_sample_sum[0] = audio_des->transmit_sample_sum[1]  = 0;
    audio_des->transmit_sample_peak[0] = audio_des->transmit_sample_peak[1] = 0;
    audio_des->transmit_sample_save = 0;
    audio_des->receive_meter_state  = METER_OFF;
    audio_des->transmit_meter_state = METER_OFF;
    audio_des->beep_bufdatalen = 0;
    audio_des->beep_bufcount = 0;
    audio_des->beep_bufptr   = (unsigned char *)0;
    audio_des->snd_minus_rcv_cnt = 0;
    audio_des->test_buffer = (unsigned char *)0;
    audio_des->beep_buffer = (unsigned char *)0;
    audio_des->beep_bufsize = 0;
    audio_des->rcurpage_cnt = 0;
    audio_des->rnxtpage_cnt = 0;
    audio_des->scurpage_cnt = 0;
    audio_des->snxtpage_cnt = 0;

    /* Do Hardware dependent initialization */

    if (id == AUDIO_1_NOBEEPER_CARD_ID || id == AUDIO_2_NOBEEPER_CARD_ID)
	audio_des->flags = AUD_F_NOBEEPER;
    else
	audio_des->flags = 0;

    if (id == AUDIO_1_CARD_ID || id == AUDIO_1_NOBEEPER_CARD_ID) {
	audio_des->audio2flag = FALSE;
	audio_des->audio_regs  = (caddr_t)audio1_regs;
	audio_des->cur_config = default_config; /* structure assignment */
	audio_des->max_rcv_bufsize = DEF_MAX_BUFFER_SIZE_1;
	audio_des->max_snd_bufsize = DEF_MAX_BUFFER_SIZE_1;
	audio_des->min_bufsize = MIN_BUFFER_SIZE_1;

	/* Make sure registers are set appropriately */

	audio1_regs->idreset = 0;
	snooze(55000);

	audio1_regs->fifointctl = IN_HALF_INT_ENA;
	audio1_regs->interrupt  = INT_ENA;

	/* Set desired initial configuration */

	audio1_init_config(audio_des);

	/* Install isr */

#ifdef __hp9000s700
	asp_isrlink(audio1_isr,IRQ_AUDIO_CORE,isc,audio_des);
	isc->gfsw->init = NULL;
	isc->gfsw->diag = NULL;
	((struct eisa_if_info *) (isc->if_info))->flags  |= INITIALIZED;
#else
	isrlink(audio1_isr,isc->int_lvl,&audio1_regs->interrupt,
		0xc0,0xc0,0x0,(int)audio_des);
#endif

    }
    else {
	audio_des->audio2flag = TRUE;
	audio_des->audio_regs  = (caddr_t)audio2_regs;
	audio_des->max_rcv_bufsize = DEF_MAX_BUFFER_SIZE_2;
	audio_des->max_snd_bufsize = DEF_MAX_BUFFER_SIZE_2;
	audio_des->min_bufsize = MIN_BUFFER_SIZE_2;

	audio_des->snd_extra_buffer = (unsigned char *)sys_memall(EXTRA_BUFSIZE);
	audio_des->rcv_extra_buffer = (unsigned char *)sys_memall(EXTRA_BUFSIZE);


	if (   audio_des->rcv_extra_buffer == (unsigned char *)0
	    || audio_des->snd_extra_buffer == (unsigned char *)0 ) {

	    panic("No memory available for audio buffers");
	}

	silence_fill(audio_des,audio_des->snd_extra_buffer,EXTRA_BUFSIZE);
	audio_des->snd_extra_ptr = audio_des->snd_extra_buffer;
	audio_des->rcv_extra_ptr = audio_des->rcv_extra_buffer;

	if (id == AUDIO_2_NOBEEPER_CARD_ID) {
	    audio_des->beep_wave_table
		= (short *)sys_memall(SINE_TABLE_SIZE * sizeof(short));

	    if (audio_des->beep_wave_table == (short *)0)
		panic("No memory available for wave table");

	    audio_des->beep_frequency = 0;
	}

	if (audio2_regs->reset == 1) {

	    snooze(100000);
	    audio2_regs->reset = 0;
	}

	/* Set desired initial configuration */

	audio2_regs->gainctl = default_gain;
	audio_des->cur_gainctl = default_gain;
	audio_des->cur_cntl = 0;
	audio2_set_config(audio_des,default_cntl);

	/* Install isr */

#ifdef __hp9000s700
	i = audio2_get_irq();
	asp_isrlink(audio2_isr,i,isc,audio_des);
	if (isc != (struct isc_table_type *)0) {
	    isc->gfsw->init = NULL;
	    isc->gfsw->diag = NULL;
	    ((struct eisa_if_info *) (isc->if_info))->flags  |= INITIALIZED;
	}
#else
#ifdef AUDIO_MAYBE_SOMEDAY
	/* This needs some thought if we want to support Series 300/400 */

	isrlink(audio2_isr,isc->int_lvl,&audio2_regs->interrupt ??,
		0x??,0x??,0x??,(int)audio_des);
#endif
#endif
    }

    if (isc != (struct isc_table_type *)0)
	isc->card_type = DIGITAL_AUDIO;

    /* Insert audio descriptor onto list */

    audio_des->next = audio_devices;
    audio_devices = audio_des;

#ifdef __hp9000s700
    if (isc == (struct isc_table_type *)0)
	return(0);

    if (audio_des->audio2flag)
	io_inform("Advanced Digital Audio Interface", isc, AUDIO_INT_LEVEL);
    else
	io_inform("Digital Audio Interface", isc, AUDIO_INT_LEVEL);
    return((*audio_saved_attach)(id,isc));
#else
    if (audio_des->audio2flag)
	return(io_inform("Advanced Digital Audio Interface", isc, AUDIO_INT_LEVEL));
    else
	return(io_inform("Digital Audio Interface", isc, AUDIO_INT_LEVEL));
#endif
}

#ifdef __hp9000s700
void
audio_link()
{
    audio_saved_attach = core_attach;
    core_attach = audio_attach;

    return;
}
#else
void
audio_link()
{
    audio_saved_make_entry = make_entry;
    make_entry = audio_make_entry;

    /* fill in configurability hooks */
    audiobeeper = audio_beeper;
    audiobeepertime = audio_beeper_time;

    return;
}
#endif

/*
 * Audio utility routines
 */

struct audio_descriptor *
audio_get_descriptor(sc)
    register int sc;
{
    register struct audio_descriptor *audio_des;

    audio_des = audio_devices;
    while (audio_des != (struct audio_descriptor *)0) {
	if (audio_des->select_code == sc)
	    break;

	audio_des = audio_des->next;
    }

    return(audio_des);
}

void
audio_output_drain(audio_des,pri)
    register struct audio_descriptor *audio_des;
    int pri;
{
    register int i;
    register struct audio1_reg *audio1_regs;
    register struct audio2_reg *audio2_regs;

    if (audio_des->audio2flag) {
	audio2_regs = (struct audio2_reg *)audio_des->audio_regs;
	i = CRIT();
	if (   audio_des->snxtpage_cnt != 0
	    || audio_des->scurpage_cnt != 0 ) {

	    audio_des->flags |= AUD_F_SNDFLUSH;
	    sleep((caddr_t)&audio2_regs->pnxtadd,pri);
	}
	UNCRIT(i);
    }
    else {
	audio1_regs = (struct audio1_reg *)audio_des->audio_regs;

	if (audio1_regs->fifostatus & OUT_EMPTY)
	    return;

	i = CRIT();
	audio1_regs->fifointctl |= OUT_EMPTY_INT_ENA;
	sleep((caddr_t)&audio1_regs->inoutfifo,pri);
	UNCRIT(i);
    }

    return;
}

void
audio2_input_drain(audio_des)
    register struct audio_descriptor *audio_des;
{
    register int i;
    register struct audio2_reg *audio2_regs;

    audio2_regs = (struct audio2_reg *)audio_des->audio_regs;
    i = CRIT();
    if (   audio_des->rnxtpage_cnt != 0
	|| audio_des->rcurpage_cnt != 0 ) {

	audio_des->flags |= AUD_F_RCVDRAIN;
	sleep((caddr_t)&audio2_regs->rnxtadd,PSWP);
    }
    UNCRIT(i);
    return;
}

void
audio_dma_stop(audio_des)
    register struct audio_descriptor *audio_des;
{
#ifdef __hp9000s700
    register int i;
    register int count;
    register struct audio2_reg *audio2_regs;

    audio2_regs = (struct audio2_reg *)audio_des->audio_regs;

    /*
     * Note that this procedure assumes that AUD_F_RCVON and
     * AUD_F_SNDON are not set and that record metering is turned off.
     * This should be insured by the calling procedure.
     */

    /*
     * We could still have up to 2 pages of data to flush, and
     * at the 5.5 Khz rate (slowest rate), this could take almost
     * two seconds. We wait up to 3 seconds here. The assumption
     * is that all the criteria in the isr will have been met, so
     * that it will clear DSTAT_IE and stop feeding the
     * hardware with buffers.
     */

    count = 0;
    i = CRIT();
    while (count < 30 && (audio2_regs->dstatus & (DSTAT_PC|DSTAT_RC)) ) {

	timeout(wakeup,(caddr_t)&audio2_regs->rnxtadd,HZ/10);
	sleep((caddr_t)&audio2_regs->rnxtadd,PSWP);
	count++;
    }

    if (count == 30) {
	UNCRIT(i);
	panic("Could not stop audio dma.");
    }

    /* Just in case ... */

    audio2_regs->dstatus &= ~DSTAT_IE;
    UNCRIT(i);

    return;
#endif
}

void
silence_fill(audio_des,buf,count)
    register struct audio_descriptor *audio_des;
    unsigned char *buf;
    int count;
{
#ifdef __hp9000s700
    register unsigned char *s;
    register int quiet;
    register int i;

    switch (audio_des->audio_format) {

    case  AUDIO_FORMAT_ULAW:
	quiet = ALAW_QUIET;
	break;

    case  AUDIO_FORMAT_ALAW:
	quiet = ULAW_QUIET;
	break;

    case  AUDIO_FORMAT_LINEAR16BIT:
	quiet = 0;
	break;
    }

    s = buf;
    i = count;
    while (i-- > 0)
	*s++ = quiet;

    fdcache(KERNELSPACE,buf,count);
    _SYNC();
    return;
#endif
}

void
audio_describe(audio_des,adptr)
    register struct audio_descriptor *audio_des;
    register struct audio_describe *adptr;
{
    register int i;

    /* Fill in common fields */

    if (audio_des->flags & AUD_F_NOBEEPER)
	adptr->flags = AD_F_NOBEEPER;
    else
	adptr->flags = 0;

    adptr->max_bits_per_sample = 16;

    /* Fill in device specific fields */

    if (audio_des->audio2flag) {
	adptr->audio_id = AUDIO_ID_CS4215;
	adptr->nrates = NSRATES;
	adptr->nchannels = 2;
	adptr->min_receive_gain  =  MIN_RX_GAIN_2;
	adptr->max_receive_gain  =  MAX_RX_GAIN_2;
	adptr->min_transmit_gain =  MIN_TX_GAIN_2;
	adptr->max_transmit_gain =  MAX_TX_GAIN_2;
	adptr->min_monitor_gain  =  MIN_MON_GAIN_2;
	adptr->max_monitor_gain  =  MAX_MON_GAIN_2;
	for (i = 0; i < NSRATES; i++)
	    adptr->sample_rate[i] = sample_rate_tab[i];
    }
    else {
	adptr->audio_id = AUDIO_ID_PSB2160;
	adptr->nrates = 1;
	adptr->nchannels = 1;
	adptr->min_receive_gain  =  MIN_GAIN_1;
	adptr->max_receive_gain  =  MAX_GAIN_1;
	adptr->min_transmit_gain =  MIN_GAIN_1;
	adptr->max_transmit_gain =  MAX_GAIN_1;
	adptr->min_monitor_gain  =  MIN_MON_GAIN_1;
	adptr->max_monitor_gain  =  MAX_MON_GAIN_1;
	adptr->sample_rate[0] = 8000;
    }

    return;
}

void
audio_set_format(audio_des,format)
    register struct audio_descriptor *audio_des;
    int format;
{
    register struct audio1_reg *audio1_regs;
    register struct audio2_reg *audio2_regs;
    register int i;
    struct psb2160conf new_config;
    int save_tx_gainval;
    unsigned long cntl;

    audio_des->audio_format = format;
    if (audio_des->audio2flag) {

	cntl = audio_des->cur_cntl & ~CNTL_DF_MASK;
	switch (format) {

	case AUDIO_FORMAT_LINEAR16BIT:
	    cntl |= CNTL_DF_LINEAR;
	    break;

	case AUDIO_FORMAT_ALAW:
	    cntl |= CNTL_DF_ALAW;
	    break;

	case AUDIO_FORMAT_ULAW:
	    cntl |= CNTL_DF_ULAW;
	    break;
	}

	audio2_set_config(audio_des,cntl);
	silence_fill(audio_des,audio_des->snd_extra_buffer,EXTRA_BUFSIZE);
	audio_des->snd_extra_ptr = audio_des->snd_extra_buffer;
    }
    else {

	audio1_regs = (struct audio1_reg *)audio_des->audio_regs;

	new_config = audio_des->cur_config; /* structure assignment */

	/* Set output gain to -infinity while we change format */

	save_tx_gainval = new_config.tx_gain;
	new_config.tx_gain = audio_gain_tab[0];
	audio1_set_config(audio_des,&new_config);

	if (format == AUDIO_FORMAT_LINEAR16BIT) {


	    i = new_config.conf3 & ~CR3_OPMODE_MASK;
	    new_config.conf3 = i | CR3_OM_LINEAR;

	    i = CRIT();
	    audio1_regs->interrupt |= DATA16_MODE;
	    audio_des->flags |= AUD_F_16BIT;
	    UNCRIT(i);
	}
	else {

	    i = new_config.conf3 & ~CR3_OPMODE_MASK;
	    new_config.conf3 = i | CR3_OM_NORMAL;

	    if (format == AUDIO_FORMAT_ALAW)
		new_config.conf4 &= ~CR4_ULAW;
	    else
		new_config.conf4 |= CR4_ULAW;

	    i = CRIT();
	    audio1_regs->interrupt &= ~DATA16_MODE;
	    audio_des->flags &= ~AUD_F_16BIT;
	    UNCRIT(i);
	}

	audio1_set_config(audio_des,&new_config);

	/* Restore gain value */

	new_config.tx_gain = save_tx_gainval;
	audio1_set_config(audio_des,&new_config);
    }

    return;
}

int
audio_set_sample_rate(audio_des,sample_rate)
    register struct audio_descriptor *audio_des;
    register int sample_rate;
{
#ifdef __hp9000s700
    register struct audio2_reg *audio2_regs;
    register int i;
    unsigned long cntl;

    if (audio_des->rcv_bufcount != 0 || audio_des->snd_bufcount != 0)
	return(EBUSY);

    audio2_regs = (struct audio2_reg *)audio_des->audio_regs;
    if (audio2_regs->dstatus & (DSTAT_PC|DSTAT_RC) )
	return(EBUSY);

    for (i = 0; i < NSRATES; i++) {
	if (sample_rate == sample_rate_tab[i])
	    break;
    }

    if (i == NSRATES)
	return(EINVAL);

    cntl = audio_des->cur_cntl & ~CNTL_RATE_MASK;
    cntl |= sample_rate_cftab[i];
    audio_des->audio_sample_rate = sample_rate;
    audio2_set_config(audio_des,cntl);
    return(0);
#endif
}

void
audio_set_channels(audio_des,nchannels)
    register struct audio_descriptor *audio_des;
    int nchannels;
{
#ifdef __hp9000s700
    unsigned long cntl;

    cntl = audio_des->cur_cntl;

    if (nchannels == 1) {
	cntl &= ~CNTL_STEREO;
	audio_des->flags &= ~AUD_F_STEREO;
    }
    else {
	cntl |= CNTL_STEREO;
	audio_des->flags |= AUD_F_STEREO;
    }

    audio2_set_config(audio_des,cntl);
    return;
#endif
}

void
audio_set_input(audio_des,in_type)
    register struct audio_descriptor *audio_des;
    int in_type;
{
#ifdef __hp9000s700
    register struct audio2_reg *audio2_regs;
    unsigned long gainctl;

    gainctl = audio_des->cur_gainctl;
    if (in_type == AUDIO_IN_LINE)
	gainctl &= ~GAIN_IS;
    else
	gainctl |= GAIN_IS;

    audio2_regs = (struct audio2_reg *)audio_des->audio_regs;
    audio2_regs->gainctl = gainctl;
    audio_des->cur_gainctl  = gainctl;
    audio_des->audio_input = in_type;
    return;
#endif
}

void
audio_set_output(audio_des,out_type)
    register struct audio_descriptor *audio_des;
    int out_type;
{
    register int i;
    struct psb2160conf new_config;
    register struct audio2_reg *audio2_regs;
    unsigned long gainctl;

    if (audio_des->audio2flag) {
	gainctl = audio_des->cur_gainctl & ~(GAIN_HE|GAIN_LE|GAIN_SE);
	if (out_type & AUDIO_OUT_INTERNAL)
	    gainctl |= GAIN_SE;

	if (out_type & AUDIO_OUT_EXTERNAL)
	    gainctl |= GAIN_HE;

	if (out_type & AUDIO_OUT_LINE)
	    gainctl |= GAIN_LE;

	audio2_regs = (struct audio2_reg *)audio_des->audio_regs;
	audio2_regs->gainctl = gainctl;
	audio_des->cur_gainctl  = gainctl;
    }
    else {
	new_config = audio_des->cur_config; /* structure assignment */
	i = new_config.conf3 & ~CR3_OUTPUT_MASK;

	switch (out_type) {

	case AUDIO_OUT_NONE:
	    new_config.conf3 = i | CR3_OUT_POR;
	    break;

	case AUDIO_OUT_INTERNAL:
	    new_config.conf3 = i | CR3_OUT_LH2;
	    break;

	case AUDIO_OUT_EXTERNAL:
	    new_config.conf3 = i | CR3_OUT_RDY;
	    break;

	case AUDIO_OUT_INTERNAL|AUDIO_OUT_EXTERNAL:
	    new_config.conf3 = i | CR3_OUT_LH3;
	    break;
	}

	audio1_set_config(audio_des,&new_config);
    }
    audio_des->audio_output = out_type;
    return;
}

void
audio1_set_gains(audio_des,rx_gain,tx_gain,mon_gain)
    register struct audio_descriptor *audio_des;
    int rx_gain;
    int tx_gain;
    int mon_gain;
{
    struct psb2160conf new_config;

    if (rx_gain < MIN_GAIN_1) {
	rx_gain = 0;
	audio_des->cur_gain[0].receive_gain = AUDIO_OFF_GAIN;
    }
    else {
	if (rx_gain > MAX_GAIN_1)
	    rx_gain = MAX_GAIN_1;

	audio_des->cur_gain[0].receive_gain = rx_gain;

	rx_gain = rx_gain - MIN_GAIN_1 + 1;
    }

    if (tx_gain < MIN_GAIN_1) {
	tx_gain = 0;
	audio_des->cur_gain[0].transmit_gain = AUDIO_OFF_GAIN;
    }
    else {
	if (tx_gain > MAX_GAIN_1)
	    tx_gain = MAX_GAIN_1;

	audio_des->cur_gain[0].transmit_gain = tx_gain;

	tx_gain = tx_gain - MIN_GAIN_1 + 1;
    }

    if (mon_gain < MIN_MON_GAIN_1) {
	mon_gain = 0;
	audio_des->cur_gain[0].monitor_gain = AUDIO_OFF_GAIN;
    }
    else {
	if (mon_gain > MAX_MON_GAIN_1)
	    mon_gain = MAX_MON_GAIN_1;

	audio_des->cur_gain[0].monitor_gain = mon_gain;

	mon_gain = mon_gain - MIN_MON_GAIN_1 + 1;
    }

    new_config = audio_des->cur_config; /* structure assignment */
    new_config.rx_gain      = audio_gain_tab[rx_gain];
    new_config.tx_gain      = audio_gain_tab[tx_gain];
    new_config.monitor_gain = monitor_gain_tab[mon_gain];

    audio1_set_config(audio_des,&new_config);
    return;
}

void
audio2_set_gains(audio_des,gainptr)
    register struct audio_descriptor *audio_des;
    register struct audio_gain *gainptr;
{
#ifdef __hp9000s700
    register struct audio2_reg *audio2_regs;
    register int channel;
    register int chan_mask;
    register int gain;
    int mon_gain;
    int tx_lgain;
    int tx_rgain;
    int rx_lgain;
    int rx_rgain;
    unsigned long gainctl;

    chan_mask = gainptr->channel_mask;

    for (channel = 0; channel < 2; channel++) {

	if (chan_mask & (1 << channel)) {

	    gain = gainptr->cgain[channel].receive_gain;
	    if (gain < MIN_RX_GAIN_2)
		gain = 0;
	    else {
		if (gain > MAX_RX_GAIN_2)
		    gain = MAX_RX_GAIN_2;
	    }
	    audio_des->cur_gain[channel].receive_gain = gain;

	    gain = gainptr->cgain[channel].transmit_gain;
	    if (gain < MIN_TX_GAIN_2)
		gain = AUDIO_OFF_GAIN;
	    else {
		if (gain > MAX_TX_GAIN_2)
		    gain = MAX_TX_GAIN_2;
	    }
	    audio_des->cur_gain[channel].transmit_gain = gain;

	    gain = gainptr->cgain[channel].monitor_gain;
	    if (gain < MIN_MON_GAIN_2)
		gain = AUDIO_OFF_GAIN;
	    else {
		if (gain > MAX_MON_GAIN_2)
		    gain = MAX_MON_GAIN_2;
	    }

	    /* Separate monitor gains are not supported */

	    audio_des->cur_gain[0].monitor_gain = gain;
	    audio_des->cur_gain[1].monitor_gain = gain;
	}
    }

    /* Compute new gain register value */

    rx_lgain = audio_des->cur_gain[0].receive_gain;
    rx_lgain = (rx_lgain * 2 + 1) / 3;

    rx_rgain = audio_des->cur_gain[1].receive_gain;
    rx_rgain = (rx_rgain * 2 + 1) / 3;

    tx_lgain = audio_des->cur_gain[0].transmit_gain;
    if (tx_lgain == AUDIO_OFF_GAIN)
	tx_lgain = 0x3f;
    else
	tx_lgain = ((9 - tx_lgain) * 2 + 1) / 3;

    tx_rgain = audio_des->cur_gain[1].transmit_gain;
    if (tx_rgain == AUDIO_OFF_GAIN)
	tx_rgain = 0x3f;
    else
	tx_rgain = ((9 - tx_rgain) * 2 + 1) / 3;

    mon_gain = audio_des->cur_gain[0].monitor_gain;
    if (mon_gain == AUDIO_OFF_GAIN)
	mon_gain = 0xf;
    else
	mon_gain = ((0 - mon_gain) + 3 ) / 6;

    gainctl = audio_des->cur_gainctl & ~( GAIN_MA_MASK
					 |GAIN_LI_MASK
					 |GAIN_RI_MASK
					 |GAIN_LO_MASK
					 |GAIN_RO_MASK );

    gainctl |= (   (mon_gain << GAIN_MA_SHIFT)
		 | (rx_lgain << GAIN_LI_SHIFT)
		 | (rx_rgain << GAIN_RI_SHIFT)
		 | (tx_lgain << GAIN_LO_SHIFT)
		 | (tx_rgain)                  );

    audio2_regs = (struct audio2_reg *)audio_des->audio_regs;
    audio2_regs->gainctl = gainctl;
    audio_des->cur_gainctl = gainctl;

    return;
#endif
}

void
audio_rcv_on(audio_des)
    register struct audio_descriptor *audio_des;
{
    register struct audio1_reg *audio1_regs;
    register struct audio2_reg *audio2_regs;

    audio_des->flags |= AUD_F_RCVON;
    if (audio_des->audio2flag) {
	audio2_regs = (struct audio2_reg *)audio_des->audio_regs;
	audio2_regs->dstatus |= DSTAT_IE;
    }
    else {
	audio1_regs = (struct audio1_reg *)audio_des->audio_regs;
	audio1_regs->interrupt |= INFIFO_ENA;
    }
    return;
}

void
audio_rcv_off(audio_des)
    register struct audio_descriptor *audio_des;
{
    register struct audio1_reg *audio1_regs;

    audio_des->flags &= ~AUD_F_RCVON;
    if (audio_des->audio2flag) {
	audio_des->flags &= ~AUD_F_RCVLASTPG;
	audio2_input_drain(audio_des);
    }
    else {
	audio1_regs = (struct audio1_reg *)audio_des->audio_regs;
	if (audio_des->receive_meter_state == METER_OFF) {
	    audio1_regs->interrupt &= ~INFIFO_ENA;
	    audio_des->flags &= ~AUD_F_ODD;
	    audio_des->receive_sample_save = 0;
	}
    }

    return;
}

void
audio1_init_config(audio_des)
    register struct audio_descriptor *audio_des;
{
    register struct psb2160conf *cur_config;
    unsigned char cmdbuf[9];
    register int i;

    cur_config = &audio_des->cur_config;

    i = cur_config->rx_gain;
    cmdbuf[0] = AC_COP_2;
    cmdbuf[1] = (i >> 8) & 0xff;
    cmdbuf[2] = i & 0xff;
    audio_send_ctl(audio_des,cmdbuf,3);

    i = cur_config->tx_gain;
    cmdbuf[0] = AC_COP_B;
    cmdbuf[1] = (i >> 8) & 0xff;
    cmdbuf[2] = i & 0xff;
    audio_send_ctl(audio_des,cmdbuf,3);

    cmdbuf[0] = AC_COP_9;
    cmdbuf[1] = cur_config->monitor_gain;
    cmdbuf[2] = cur_config->tone_gain1;
    cmdbuf[3] = cur_config->tone_gain2;
    cmdbuf[4] = cur_config->tone_gain3;
    audio_send_ctl(audio_des,cmdbuf,5);

    cmdbuf[0] = AC_SOP_0;
    cmdbuf[1] = cur_config->conf4;
    cmdbuf[2] = cur_config->conf3;
    cmdbuf[3] = cur_config->conf2;
    cmdbuf[4] = cur_config->conf1;
    audio_send_ctl(audio_des,cmdbuf,5);

    return;
}

/*
 * audio1_set_config checks a new configuration against
 * the current one, and issues commands to change the
 * configuration to match the new configuration.
 * The updated new configuration is stored in the
 * audio descriptor.
 *
 */

void
audio1_set_config(audio_des,new_config)
    register struct audio_descriptor *audio_des;
    register struct psb2160conf *new_config;
{
    register struct psb2160conf *cur_config;
    register int i;
    register int j;
    unsigned char cmdbuf[9];

    cur_config = &audio_des->cur_config;

    /* Check each element against the current configuration */

    if ((i = new_config->rx_gain) != cur_config->rx_gain) {
	cmdbuf[0] = AC_COP_2;
	cmdbuf[1] = (i >> 8) & 0xff;
	cmdbuf[2] = i & 0xff;
	audio_send_ctl(audio_des,cmdbuf,3);
	cur_config->rx_gain = i;
    }

    if ((i = new_config->tx_gain) != cur_config->tx_gain) {
	cmdbuf[0] = AC_COP_B;
	cmdbuf[1] = (i >> 8) & 0xff;
	cmdbuf[2] = i & 0xff;
	audio_send_ctl(audio_des,cmdbuf,3);
	cur_config->tx_gain = i;
    }

    if (   (i = new_config->tone_time1) != cur_config->tone_time1
	|| (j = new_config->tone_freq1) != cur_config->tone_freq1 ) {

	cmdbuf[0] = AC_COP_1;
	cmdbuf[1] = (i >> 8) & 0xff;
	cmdbuf[2] = i & 0xff;
	cmdbuf[3] = (j >> 8) & 0xff;
	cmdbuf[4] = j & 0xff;
	audio_send_ctl(audio_des,cmdbuf,5);
	cur_config->tone_time1 = i;
	cur_config->tone_freq1 = j;
    }

    if (   (i = new_config->tone_time2) != cur_config->tone_time2
	|| (j = new_config->tone_freq2) != cur_config->tone_freq2 ) {

	cmdbuf[0] = AC_COP_3;
	cmdbuf[1] = (i >> 8) & 0xff;
	cmdbuf[2] = i & 0xff;
	cmdbuf[3] = (j >> 8) & 0xff;
	cmdbuf[4] = j & 0xff;
	audio_send_ctl(audio_des,cmdbuf,5);
	cur_config->tone_time2 = i;
	cur_config->tone_freq2 = j;
    }

    if (   (i = new_config->tone_time3) != cur_config->tone_time3
	|| (j = new_config->tone_freq3) != cur_config->tone_freq3 ) {

	cmdbuf[0] = AC_COP_E;
	cmdbuf[1] = (i >> 8) & 0xff;
	cmdbuf[2] = i & 0xff;
	cmdbuf[3] = (j >> 8) & 0xff;
	cmdbuf[4] = j & 0xff;
	audio_send_ctl(audio_des,cmdbuf,5);
	cur_config->tone_time3 = i;
	cur_config->tone_freq3 = j;
    }

    if (   new_config->tone_gain1 != cur_config->tone_gain1
	|| new_config->tone_gain2 != cur_config->tone_gain2
	|| new_config->tone_gain3 != cur_config->tone_gain3
	|| new_config->monitor_gain != cur_config->monitor_gain ) {

	cmdbuf[0] = AC_COP_9;
	cmdbuf[1] = cur_config->monitor_gain = new_config->monitor_gain;
	cmdbuf[2] = cur_config->tone_gain3 = new_config->tone_gain3;
	cmdbuf[3] = cur_config->tone_gain2 = new_config->tone_gain2;
	cmdbuf[4] = cur_config->tone_gain1 = new_config->tone_gain1;
	audio_send_ctl(audio_des,cmdbuf,5);
    }

    if (   new_config->dtmf_high != cur_config->dtmf_high
	|| new_config->dtmf_low  != cur_config->dtmf_low ) {

	cmdbuf[0] = AC_COP_8;
	cmdbuf[1] = cur_config->dtmf_high = new_config->dtmf_high;
	cmdbuf[2] = cur_config->dtmf_low  = new_config->dtmf_low;
	audio_send_ctl(audio_des,cmdbuf,3);
    }

    if (   new_config->fr[8] != cur_config->fr[8]
	|| new_config->fr[9] != cur_config->fr[9]
	|| new_config->fx[8] != cur_config->fx[8]
	|| new_config->fx[9] != cur_config->fx[9] ) {

	cmdbuf[0] = AC_COP_D;
	cmdbuf[1] = cur_config->fx[8] = new_config->fx[8];
	cmdbuf[1] = cur_config->fx[9] = new_config->fx[9];
	cmdbuf[1] = cur_config->fr[8] = new_config->fr[8];
	cmdbuf[1] = cur_config->fr[9] = new_config->fr[9];
	audio_send_ctl(audio_des,cmdbuf,5);
    }

    if (bcmp(new_config->fr,cur_config->fr,8) != 0) {
	cmdbuf[0] = AC_COP_A;
	bcopy(new_config->fr,&cmdbuf[1],8);
	audio_send_ctl(audio_des,cmdbuf,9);
	bcopy(new_config->fr,cur_config->fr,8);
    }

    if (bcmp(new_config->fx,cur_config->fx,8) != 0) {
	cmdbuf[0] = AC_COP_C;
	bcopy(new_config->fx,&cmdbuf[1],8);
	audio_send_ctl(audio_des,cmdbuf,9);
	bcopy(new_config->fx,cur_config->fx,8);
    }

    if (new_config->conf1 != cur_config->conf1) {
	cmdbuf[0] = AC_SOP_4;
	cmdbuf[1] = cur_config->conf1 = new_config->conf1;
	audio_send_ctl(audio_des,cmdbuf,2);
    }

    if (new_config->conf2 != cur_config->conf2) {
	cmdbuf[0] = AC_SOP_5;
	cmdbuf[1] = cur_config->conf2 = new_config->conf2;
	audio_send_ctl(audio_des,cmdbuf,2);
    }

    if (new_config->conf3 != cur_config->conf3) {
	cmdbuf[0] = AC_SOP_6;
	cmdbuf[1] = cur_config->conf3 = new_config->conf3;
	audio_send_ctl(audio_des,cmdbuf,2);
    }

    if (new_config->conf4 != cur_config->conf4) {
	cmdbuf[0] = AC_SOP_7;
	cmdbuf[1] = cur_config->conf4 = new_config->conf4;
	audio_send_ctl(audio_des,cmdbuf,2);
    }

    return;
}

void
audio2_set_config(audio_des,cntl)
    register struct audio_descriptor *audio_des;
    unsigned long cntl;
{
#ifdef __hp9000s700
    register struct audio2_reg *audio2_regs;
    register int h;
    register int i;
    register int j;
    register int k;

    if (audio_des->cur_cntl == cntl)
	return;

    audio2_regs = (struct audio2_reg *)audio_des->audio_regs;

    /* Disable Output */

    audio2_regs->gainctl = audio_des->cur_gainctl & ~(GAIN_HE|GAIN_LE|GAIN_SE);

    /* Workaround for analog devices part, write control register twice */
    /* with 500 usec wait between writes. We also do the first 500 usec */
    /* wait to minimize click after having just muted the output.       */

    for (h = 0; h < 2; h++) {

	snooze(500);

	/* Note that Rev C of the Crystal chip needs all of these hacks. */
	/* Supposedly, Rev C will not be used in any customer shipped    */
	/* boxes, so this hack can eventually be removed.                */

	for (i = 0; i < 3; i++) {

	    for (j = 0; j < 20; j++) {

		audio2_regs->cntl = cntl;

		k = 0;
		while (k < 20 && (audio2_regs->cntl & CNTL_STATUS)) {
		    k++;
		    snooze(100);
		}

		if (k < 20)
		    break;

		/*
		 * The hardware lunched (known hardware defect).
		 * We can usually recover with a reset.
		 */

		audio2_regs->reset = 1;
		snooze(100000);
		audio2_regs->reset = 0;
		snooze(1000);

		k = 0;
		while (k < 20 && (audio2_regs->cntl & CNTL_STATUS)) {
		    k++;
		    snooze(100);
		}

		if (k == 20)
		    panic("Audio hardware control bit stuck after reset");
	    }

	    if (j == 20)
		panic("Audio hardware failed. Not recoverable with reset");

	    /* OK, Now lets make really sure we haven't lunched the chip */
	    /* Make sure we are seeing clocks in the diag register.      */

	    snooze(1000);
	    for (j = 0; j < 100 && audio2_regs->diag == 0; j++)
		;

	    if (j < 100)
		break;
	}

	if (i == 3)
	    panic("Audio hardware clock dead.\n");
    }

    audio_des->cur_cntl = cntl;

    /* Restore Output */

    snooze(1000);
    audio2_regs->gainctl = audio_des->cur_gainctl;

    /* Clear Overrange register */

    audio2_regs->ovrange = 0;

    return;
#endif
}


void
audio_send_ctl(audio_des,cmdbuf,cmdlen)
    register struct audio_descriptor *audio_des;
    unsigned char *cmdbuf;
    int cmdlen;
{
    register int i;
    register int j;
    register unsigned char *s;
    register struct audio1_reg *audio1_regs;

    audio1_regs = (struct audio1_reg *)audio_des->audio_regs;

    /* Make sure control fifo is empty */

    if ((audio1_regs->fifostatus & CTL_EMPTY) == 0) {

	/*
	 * It is not, make sure it is enabled, and
	 * wait for it to empty.
	 */

	if ((audio1_regs->interrupt & CTLFIFO_ENA) == 0) {
	    i = CRIT();
	    audio1_regs->interrupt |= CTLFIFO_ENA;
	    UNCRIT(i);
	}

	/* Wait for control fifo to empty */

	for (i = 0; i < CTLFIFO_SIZE; i++) {

	    snooze(150);
	    if ((audio1_regs->fifostatus & CTL_EMPTY) != 0)
		break;
	}

	if (i >= CTLFIFO_SIZE)
	    panic("audio control fifo failure");

    }

    /*
     * Disable control fifo so that we can guarantee
     * that a complete command will be loaded before
     * enabling it.
     */

    i = CRIT();
    audio1_regs->interrupt &= ~CTLFIFO_ENA;

    /* Load command into control fifo */

    s = cmdbuf;
    j = cmdlen;
    while (j-- > 0)
	audio1_regs->ctlfifo = *s++;

    /* Re-enable control fifo */

    audio1_regs->interrupt |= CTLFIFO_ENA;
    UNCRIT(i);

    return;
}

#ifdef __hp9000s700
void
audio2_meter_sum(audio_des,send_flag,isr_flag,nbytes)
    register struct audio_descriptor *audio_des;
    int send_flag;
    int isr_flag;
    int nbytes;
{
    register int sum0;
    register int sum1;
    register int peak0;
    register int peak1;
    register unsigned char *s1;
    register short *s2;
    register unsigned short *conv_tab;
    register int cur_data;
    register int i;
    int nsamples;

    sum0 = 0;
    sum1 = 0;
    nsamples = 0;
    if (send_flag == FALSE) {
	peak0 = audio_des->receive_sample_peak[0];
	peak1 = audio_des->receive_sample_peak[1];
	s1 = audio_des->rcv_meter_ptr;
    }
    else {
	peak0 = audio_des->transmit_sample_peak[0];
	peak1 = audio_des->transmit_sample_peak[1];
	s1 = audio_des->snd_meter_ptr;
    }

    if (nbytes > 0 && nbytes <= NBPG && (audio_des->flags & AUD_F_MTRNOSUM) == 0) {
	if (audio_des->audio_format == AUDIO_FORMAT_LINEAR16BIT) {
	    s2 = (short *)s1;
	    if ((audio_des->flags & AUD_F_STEREO) == 0) {
		nsamples = i = nbytes >> 1;
		while (i-- > 0) {
		    cur_data = *s2++;
		    if (cur_data < 0)
			cur_data = -cur_data;
		    sum0 += cur_data;
		    if (cur_data > peak0)
			peak0 = cur_data;
		}
	    }
	    else {
		nsamples = i = nbytes >> 2;
		while (i-- > 0) {
		    cur_data = *s2++;
		    if (cur_data < 0)
			cur_data = -cur_data;
		    sum0 += cur_data;
		    if (cur_data > peak0)
			peak0 = cur_data;
		    cur_data = *s2++;
		    if (cur_data < 0)
			cur_data = -cur_data;
		    sum1 += cur_data;
		    if (cur_data > peak1)
			peak1 = cur_data;
		}
	    }
	    s1 = (unsigned char *)s2;
	}
	else {
	    if (audio_des->audio_format == AUDIO_FORMAT_ULAW)
		conv_tab = ulaw_tab;
	    else
		conv_tab = alaw_tab;

	    if ((audio_des->flags & AUD_F_STEREO) == 0) {
		nsamples = i = nbytes;
		while (i-- > 0) {
		    cur_data = conv_tab[*s1++ & 0x7f];
		    sum0 += cur_data;
		    if (cur_data > peak0)
			peak0 = cur_data;
		}
	    }
	    else {
		nsamples = i = nbytes >> 1;
		while (i-- > 0) {
		    cur_data = conv_tab[*s1++ & 0x7f];
		    sum0 += cur_data;
		    if (cur_data > peak0)
			peak0 = cur_data;
		    cur_data = conv_tab[*s1++ & 0x7f];
		    sum1 += cur_data;
		    if (cur_data > peak1)
			peak1 = cur_data;
		}
	    }
	}
    }

    if (send_flag == FALSE) {
	audio_des->receive_sample_peak[0] = peak0;
	audio_des->receive_sample_peak[1] = peak1;
	audio_des->receive_sample_sum[0] += sum0;
	audio_des->receive_sample_sum[1] += sum1;
	audio_des->receive_nsamples += nsamples;
	audio_des->rcv_meter_ptr = s1;

	/*
	 * We only update the meter pointer if isr_flag is true (indicating
	 * we are at a page boundary). This means that this code should
	 * always get called, even if nbytes == 0.
	 */

	/* Update meter pointer */

	if (audio_des->rcurpage_cnt == 0) {
	    if (isr_flag && audio_des->rnxtpage_cnt != 0)
		audio_des->rcv_meter_ptr = audio_des->rcv_buftail;
	    else {
		if (audio_des->rcv_meter_ptr == audio_des->rcv_extra_buffer + (2 * NBPG))
		    audio_des->rcv_meter_ptr = audio_des->rcv_extra_buffer;
	    }
	}
	else {
	    if (isr_flag && audio_des->rnxtpage_cnt == 0)
		audio_des->rcv_meter_ptr = audio_des->rcv_extra_ptr;
	    else {
		if (audio_des->rcv_meter_ptr == audio_des->rcv_buffer + audio_des->rcv_bufsize)
		    audio_des->rcv_meter_ptr = audio_des->rcv_buffer;
	    }
	}
    }
    else {
	audio_des->transmit_sample_peak[0] = peak0;
	audio_des->transmit_sample_peak[1] = peak1;
	audio_des->transmit_sample_sum[0] += sum0;
	audio_des->transmit_sample_sum[1] += sum1;
	audio_des->transmit_nsamples += nsamples;
	audio_des->snd_meter_ptr = s1;
	if (audio_des->snd_meter_ptr == audio_des->snd_buffer + audio_des->snd_bufsize)
	    audio_des->snd_meter_ptr = audio_des->snd_buffer;
    }

    return;
}

struct {
    char *model;
    int  irq;
} audio2_sgc_irqinfo[] = {
    { "9000/720",   IRQ_SGC_SLOT1 },
    { "9000/730",   IRQ_SGC_SLOT1 },
    { "9000/750",   IRQ_SGC_SLOT2 },
    { "9000/755",   IRQ_SGC_SLOT2 },
    { "9000/---",   IRQ_SGC_SLOT2 }, /* Spare to patch via adb */
    { (char *)0,    -1 }
};

int
audio2_get_irq()
{
    register int i;

    for (i = 0; audio2_sgc_irqinfo[i].model != (char *)0; i++) {
	if (bcmp(audio2_sgc_irqinfo[i].model,utsname.machine,8) == 0)
	    break;
    }

    if (audio2_sgc_irqinfo[i].model == (char *)0)
	return(IRQ_AUDIO_CORE);

    return(audio2_sgc_irqinfo[i].irq);
}
#endif

int
audio_set_beep(audio_des,abtptr)
    register struct audio_descriptor *audio_des;
    register struct audio_beep_type *abtptr;
{
    register int i;
    unsigned char *bufptr;

    /*
     * For now, we only support:
     *      type == AUDIO_BEEP_TONE or type == AUDIO_BEEP_ULAW
     *      if type == AUDIO_BEEP_ULAW:
     *          repeat_count == 1
     *          sample_rate == 8000
     *          nchannels == 1
     */

    if (abtptr->type > AUDIO_BEEP_ULAW)
	return(EINVAL);

    if (   abtptr->type == AUDIO_BEEP_ULAW
	&& (   abtptr->repeat_count != 1
	    || abtptr->sample_rate != 8000
	    || abtptr->nchannels != 1) ) {

	return(EINVAL);
    }

    if (abtptr->type == AUDIO_BEEP_TONE) {
	i = CRIT();
	audio_des->flags &= ~AUD_F_SOUNDBEEP;
	UNCRIT(i);
    }
    else {

	/* If size is greater than max, return EINVAL */

	i = abtptr->datalen;
	if (i > audio_des->max_snd_bufsize)
	    return(EINVAL);

	/* If size is larger than previous, allocate new buffer */

	if (i > audio_des->beep_bufsize) {

	    /* Free old buffer if one exists */

	    if (audio_des->beep_bufsize > 0)
		sys_memfree(audio_des->beep_buffer,audio_des->beep_bufsize);

	    /* Round size up to next 4k boundary */

	    i = (i + PGOFSET) & ~PGOFSET;
	    bufptr = (unsigned char *)sys_memall(i);
	    if (bufptr == (unsigned char *)0) {
		i = CRIT();
		audio_des->flags &= ~AUD_F_SOUNDBEEP;
		audio_des->beep_bufsize = 0;
		audio_des->beep_bufdatalen = 0;
		UNCRIT(i);
		return(ENOMEM);
	    }

	    audio_des->beep_buffer = bufptr;
	    audio_des->beep_bufsize = i;
	}

	audio_des->beep_bufdatalen = 0; /* In case copyin fails */

	i = copyin(abtptr->data,audio_des->beep_buffer,abtptr->datalen);
	if (i != 0)
	    return(i);

	audio_des->beep_bufdatalen = abtptr->datalen;
	i = CRIT();
	audio_des->flags |= AUD_F_SOUNDBEEP;
	UNCRIT(i);
    }
    return(0);
}

void
audio_beeper(audio_des,frequency,attenuation,duration,beep_type)
    register struct audio_descriptor *audio_des;
    unsigned int frequency;
    unsigned int attenuation;
    unsigned int duration;
    int beep_type;
{
    register struct audio1_reg *audio1_regs;

    if (audio_des == (struct audio_descriptor *)0) {

	/* Use first audio device */

	if ((audio_des = audio_devices) == (struct audio_descriptor *)0)
	    return;

	/* Don't beep if read/write is device open */

	if (audio_des->flags & AUD_F_RWOPEN)
	    return;

    }
    else {

	/* This was an explicit request to this device. We don't beep */
	/* if we are busy or paused.                                  */

	if (   (audio_des->flags & AUD_F_SNDPAUSE) != 0
	    || audio_des->snd_bufcount > 0 ) {

	    return;
	}

	if (audio_des->audio2flag) {
	    if ((audio_des->flags & AUD_F_SNDON) || (audio_des->flags & AUD_F_RCVON))
		return;
	}
	else {
	    audio1_regs = (struct audio1_reg *)audio_des->audio_regs;
	    if ( (audio1_regs->fifostatus & OUT_EMPTY) == 0 )
		return;
	}
    }

    if (audio_des->audio2flag) {

	/*
	 * Don't beep if metering is on. Since metering can be
	 * done from the control device, we check here in the
	 * common code.
	 */

	if (   audio_des->receive_meter_state  != METER_OFF
	    || audio_des->transmit_meter_state != METER_OFF ) {

	    return;
	}
	audio2_beeper(audio_des,frequency,attenuation,duration,beep_type);
    }
    else
	audio1_beeper(audio_des,frequency,attenuation,duration,beep_type);

    return;
}

void
audio1_beeper(audio_des,frequency,attenuation,duration,beep_type)
    register struct audio_descriptor *audio_des;
    unsigned int frequency;
    unsigned int attenuation;
    unsigned int duration;
    int beep_type;
{
    register int i;
    register int x;
    register struct audio1_reg *audio1_regs;
    struct psb2160conf new_config;

    audio1_regs = (struct audio1_reg *)audio_des->audio_regs;


    /* Check to see if currently beeping */

    x = CRIT();
    if (audio_des->flags & AUD_F_BEEP) {

	/* stop  beeping */

	beeper_off(audio_des);
    }
    UNCRIT(x);

    switch (beep_type) {

    case BEEPTYPE_200:

	/*
	 * If old Series 200 beeper format, convert frequency and duration
	 * to Series 300 beeper format
	 */

	/* Series 200 duration is 2's complement of Series 300 */

	if (duration != 0)
	    duration = 256 - duration;

	/* Convert duration to ticks */

	duration = ((duration * 10 * HZ) + 999) / 1000;

	/* Convert Series 200 frequency */

	frequency = (frequency & 0x3f) + 1;
	frequency = (1024 + (frequency / 2)) / frequency;
	if (frequency > 1023)
	    frequency = 1023;

	if (audio_des->flags & AUD_F_SOUNDBEEP)
	    attenuation = soundbeep_gain_tab[attenuation];
	else
	    attenuation = beep_gain_tab[attenuation];
	break;

    case BEEPTYPE_300:

	/* Convert duration to ticks */

	duration = ((duration * 10 * HZ) + 999) / 1000;
	if (audio_des->flags & AUD_F_SOUNDBEEP)
	    attenuation = soundbeep_gain_tab[attenuation];
	else
	    attenuation = beep_gain_tab[attenuation];
	break;

    case BEEPTYPE_GENERAL:

	/* For other beep types, duration == 0 means   */
	/* turn on beeper until explicitly turned off. */
	/* However, for this type it means "off". We   */
	/* just return, since we turned the beeper off */
	/* when we entered this routine.               */

	if (duration == 0)
	    return;

	/* Convert duration to ticks */

	duration = ((duration * HZ) + 999) / 1000;

	if (attenuation > 100)
	    attenuation = 100;

	if (audio_des->flags & AUD_F_SOUNDBEEP)
	    attenuation = soundbeep_gain_tab[(105 - attenuation)/7];
	else
	    attenuation = monitor_gain_tab[(((attenuation + 2) * 2) / 5)];

	/* Convert frequency to Series 300 frequency index */

	if (frequency == 0)
	    frequency = 1;

	frequency = (83333 + (frequency / 2)) / frequency;
	if (frequency > 1023)
	    frequency = 1023;
	break;
    }

    /* Don't bother if gain is -infinity */

    if (attenuation == 0x88)
	return;

    new_config = audio_des->cur_config; /* structure assignment */

    x = CRIT();
    if ((audio_des->flags & AUD_F_BEEPCONF) == 0) {

	/* Save current configuration in beep_save_config */

	if (audio1_regs->interrupt & DATA16_MODE)
	    audio_des->beep_save_16bit_flag = TRUE;
	else
	    audio_des->beep_save_16bit_flag = FALSE;

	audio_des->beep_save_config = audio_des->cur_config; /* structure assignment */
    }

    /* Set output to Speaker Only */

    i = new_config.conf3 & ~CR3_OUTPUT_MASK;
    new_config.conf3 = i | CR3_OUT_LH2;

    if (audio_des->flags & AUD_F_SOUNDBEEP) {

	new_config.tx_gain = attenuation;

	i = new_config.conf3 & ~CR3_OPMODE_MASK;
	new_config.conf3 = i | CR3_OM_NORMAL;
	new_config.conf4 |= CR4_ULAW;

	audio_des->beep_bufcount = audio_des->beep_bufdatalen;
	audio_des->beep_bufptr = audio_des->beep_buffer;
	audio1_regs->interrupt &= ~DATA16_MODE;
	audio1_regs->fifointctl |= OUT_HALF_INT_ENA;
	audio1_set_config(audio_des,&new_config);
    }
    else {

	/* Set and clear appropriate Tone Generator flags */

	i = new_config.conf4 & ~(CR4_PM|CR4_BM|CR4_BT|CR4_TM);
	new_config.conf4 = i | CR4_TG;

	/* Set frequency coefficient */

	new_config.tone_freq1 = beep_freq_tab[frequency];

	/* Set TG Gain */

	new_config.tone_gain1 = attenuation;

	/* Start tone generator */

	audio1_set_config(audio_des,&new_config);
	if (duration != 0)
	    timeout(beeper_off,audio_des,duration);
    }
    audio_des->flags |= (AUD_F_BEEP|AUD_F_BEEPCONF);
    UNCRIT(x);

    return;
}

void
audio2_beeper(audio_des,frequency,attenuation,duration,beep_type)
    register struct audio_descriptor *audio_des;
    unsigned int frequency;
    unsigned int attenuation;
    unsigned int duration;
    int beep_type;
{
    register int i;
    register int j;
    register int h;
    register int save_gain;
    register int value;
    register struct audio2_reg *audio2_regs;

    audio2_regs = (struct audio2_reg *)audio_des->audio_regs;

    /* Check to see if currently beeping */

    i = CRIT();
    if (audio_des->flags & AUD_F_BEEP) {

	/* stop  beeping */

	beeper_off(audio_des);
    }
    UNCRIT(i);

    switch (beep_type) {

    case BEEPTYPE_200:

	/*
	 *  Series 200 beeper format
	 */

	/* Series 200 duration is 2's complement in 100ths of second. */
	/* convert to milliseconds.                                   */

	if (duration != 0)
	    duration = (256 - duration) * 10;

	/* Convert Series 200 frequency to hertz */

	frequency = (frequency & 0x3f) + 1;
	frequency = (1024 + (frequency / 2)) / frequency;
	if (frequency > 1023)
	    frequency = 1023;

	if (frequency == 0)
	    frequency = 1;

	frequency = (83333 + (frequency / 2)) / frequency;

	/* If attenuation is -infinity, just return */

	if (attenuation == 15)
	    return;

	/* Convert attenuation to audio II format */

	attenuation = (attenuation * 4 + 1) / 3;
	break;

    case BEEPTYPE_300:

	/* Convert duration to milliseconds */

	duration *= 10;

	/* Convert frequency index to hertz */

	if (frequency == 0)
	    frequency = 1;

	frequency = (83333 + (frequency / 2)) / frequency;

	/* If attenuation is -infinity, just return */

	if (attenuation == 15)
	    return;

	/* Convert attenuation to audio II format */

	attenuation = (attenuation * 4 + 1) / 3;
	break;

    case BEEPTYPE_GENERAL:

	/* For other beep types, duration == 0 means   */
	/* turn on beeper until explicitly turned off. */
	/* However, for this type it means "off". We   */
	/* just return, since we turned the beeper off */
	/* when we entered this routine.               */

	if (duration == 0)
	    return;

	if (attenuation > 100)
	    attenuation = 100;

	/* If attenuation is -infinity, just return */

	if (attenuation == 0)
	    return;

	/* Convert attenuation to audio II format */

	attenuation = (50 - (attenuation / 2));
	break;
    }


    /*
     *
     * Set output to Speaker Only, No input gain, no monitor gain,
     * and specified output gain.
     */

    save_gain = audio_des->cur_gainctl; /* save temporarily */
    i = ( GAIN_SE | 0x00f00000 | attenuation << 6 | attenuation);
    audio2_regs->gainctl = i;
    audio_des->cur_gainctl = i;

    if ((audio_des->flags & AUD_F_BEEPCONF) == 0) {

	/* Save current configuration */

	audio_des->beep_save_cntl = audio_des->cur_cntl;
	audio_des->beep_save_gain = save_gain;

	/* Set appropriate beep configuration */

	if (audio_des->flags & AUD_F_SOUNDBEEP) {

	    /*
	     * Set Format to U-law, rate to 8Khz, channels to 1
	     */

	    audio2_set_config(audio_des,(CNTL_DF_ULAW|CNTL_RATE_8000));
	}
	else {

	    /*
	     * Set Format to 16 bit linear, rate to 32Khz, channels to 1
	     */

	    audio2_set_config(audio_des,(CNTL_DF_LINEAR|CNTL_RATE_32000));
	}
    }

    if (audio_des->flags & AUD_F_SOUNDBEEP) {
	audio_des->beep_bufcount = audio_des->beep_bufdatalen;
	audio_des->beep_bufptr = audio_des->beep_buffer;
    }
    else {

	VASSERT(audio_des->beep_wave_table != 0);

	/*
	 * Convert duration in milliseconds to number of samples.
	 * If duration is zero, this means to keep beeping until
	 * explicity shut off. We do the next best thing, by setting
	 * the number of samples to the maximum positive integer.
	 *
	 * Note that we use the beep_bufcount field to count the
	 * number of samples to generate.
	 */

	if (duration == 0 || duration >= 0x04000000)
	    audio_des->beep_bufcount = 0x7fffffff;
	else
	    audio_des->beep_bufcount = 32 * duration;

	if (frequency < 40)
	    frequency = 40;
	if (frequency > 15000)
	    frequency = 15000;

	if (frequency != audio_des->beep_frequency) {

	    /*
	     * Create band limited square wave table from sine table.
	     * We add in as many harmonics that the current frequency
	     * allows.
	     */

	    for (i = 0; i < SINE_TABLE_SIZE; i++) {
		value = sine_wave_table[i] * 10000;
		for (j = 1, h = 3; (h * frequency) < 16000; j++, h += 2) {
		    value += ((sine_wave_table[(i * h) & SINE_TABLE_MASK] * 10000 + j) / h);
		}
		audio_des->beep_wave_table[i] = (value + 5000) / 10000;
	    }
	}

	audio_des->beep_frequency = frequency;
	audio_des->beep_accum_step = 0;
    }

    i = CRIT();
    audio_des->flags |= (AUD_F_BEEP|AUD_F_BEEPCONF);
    audio2_regs->dstatus |= DSTAT_IE;
    UNCRIT(i);

    return;
}

void
beeper_off(audio_des)
    register struct audio_descriptor *audio_des;
{
    register int i;
    register struct audio1_reg *audio1_regs;

    i = CRIT();

    /* Check to see if currently beeping */

    if (audio_des->flags & AUD_F_BEEP) {

	if (audio_des->audio2flag)
	    audio_des->beep_bufcount = 0;
	else {
	    audio1_regs = (struct audio1_reg *)audio_des->audio_regs;

	    if (audio_des->flags & AUD_F_SOUNDBEEP) {
		audio_des->beep_bufcount = 0;
		audio1_regs->fifointctl &= ~OUT_HALF_INT_ENA;
	    }
	    else {

		/* Call untimeout since we don't know if we     */
		/* were called directly or due to time expiring */

		untimeout(beeper_off,audio_des);
	    }

	    /* Stop beep by restoring configuration */

	    audio1_set_config(audio_des,&(audio_des->beep_save_config));
	}
	audio_des->flags &= ~AUD_F_BEEP;
    }
    UNCRIT(i);

    return;
}

void
beeper_reset(audio_des,open_flag)
    register struct audio_descriptor *audio_des;
    int open_flag;
{
    register struct audio2_reg *audio2_regs;
    register struct audio1_reg *audio1_regs;
    register int i;

    /* Check to see if currently beeping */

    i = CRIT();
    if (audio_des->flags & AUD_F_BEEP) {

	/* stop  beeping */

	beeper_off(audio_des);
    }

    /* Restore configuration */

    if (audio_des->audio2flag) {

	audio_dma_stop(audio_des);

	silence_fill(audio_des,audio_des->snd_extra_buffer,EXTRA_BUFSIZE);
	audio_des->snd_extra_ptr = audio_des->snd_extra_buffer;

	audio2_regs = (struct audio2_reg *)audio_des->audio_regs;
	if (open_flag) {
	    audio2_regs->gainctl = default_gain;
	    audio_des->cur_gainctl = default_gain;
	    audio2_set_config(audio_des,default_cntl);
	}
	else {
	    audio2_set_config(audio_des,audio_des->beep_save_cntl);
	    audio2_regs->gainctl = audio_des->beep_save_gain;
	    audio_des->cur_gainctl = audio_des->beep_save_gain;
	}
    }
    else {

	if (open_flag)
	    audio1_set_config(audio_des,&default_config);
	else {

	    /*
	     * Configuration was partly restored by beeper_off, we
	     * just need to restore 8/16 bit data state here.
	     */

	    if (audio_des->beep_save_16bit_flag) {
		audio1_regs = (struct audio1_reg *)audio_des->audio_regs;
		audio1_regs->interrupt |= DATA16_MODE;
	    }
	}
    }

    audio_des->flags &= ~AUD_F_BEEPCONF;
    UNCRIT(i);

    return;
}

audio_beeper_time(audio_des)
    register struct audio_descriptor *audio_des;
{
    register int i;
    register int timeleft;

    if (audio_des == (struct audio_descriptor *)0) {

	/* Use first audio device */

	if ((audio_des = audio_devices) == (struct audio_descriptor *)0)
	    return(0);
    }

    /* If device is not beeping, just return 0 */

    i = CRIT();
    if ((audio_des->flags & AUD_F_BEEP) == 0) {
	UNCRIT(i);
	return(0);
    }

    if (audio_des->flags & AUD_F_SOUNDBEEP) {
	timeleft = audio_des->beep_bufcount;
	UNCRIT(i);
	timeleft = ((timeleft * 100) + 8000 - 1) / 8000;
    }
    else {
	if (audio_des->audio2flag) {
	    timeleft = audio_des->beep_bufcount;
	    UNCRIT(i);
	    timeleft = ((timeleft * 100) + 32000 - 1) / 32000;
	}
	else {
	    timeleft = find_timeout(beeper_off,audio_des,0);
	    UNCRIT(i);

	    if (timeleft < 0)
		timeleft = 0;
	    else {
		if (timeleft == 0)
		    timeleft = 1;
		else
		    timeleft = ((timeleft * 100) + HZ - 1) / HZ;
	    }
	}
    }

    return(timeleft);
}

#ifdef __hp9000s700
void
beep_fill(audio_des)
    register struct audio_descriptor *audio_des;
{
    register int i;
    register int sample;
    register int remainder;
    register int wave_step;
    register int accum_step;
    register short *sptr;
    register unsigned char *cptr;

    if ((i = audio_des->beep_bufcount) == 0)
	beeper_off(audio_des);

    if (audio_des->flags & AUD_F_SOUNDBEEP) {
	if ( i > 0) {

	    if (i > NBPG)
		i = NBPG;

	    bcopy(audio_des->beep_bufptr,audio_des->snd_extra_ptr,i);
	    audio_des->beep_bufptr += i;
	    audio_des->beep_bufcount -= i;
	}

	if (i < NBPG) {
	    cptr = audio_des->snd_extra_ptr + i;
	    i = NBPG - i;
	    while (i-- > 0)
		*cptr++ = ULAW_QUIET;
	}
    }
    else {
	sptr = (short *)audio_des->snd_extra_ptr;
	if (i > 0) {
	    wave_step = 20 * audio_des->beep_frequency;
	    accum_step = audio_des->beep_accum_step;
	    if (i > (NBPG / 2))
		i = NBPG / 2;

	    while (i-- > 0) {
		if (accum_step >= 640000)
		    accum_step -= 640000;
		sample = accum_step / 10000;
		remainder = accum_step - sample * 10000;
		*sptr++ = (   (10000 - remainder) * audio_des->beep_wave_table[sample]
			    + remainder *  audio_des->beep_wave_table[sample + 1]
			    + 5000
			  ) / 10000;

		accum_step += wave_step;
	    }

	    audio_des->beep_accum_step = accum_step;

	    i = sptr - (short *)audio_des->snd_extra_ptr;
	    audio_des->beep_bufcount -= i;
	}

	/* Fill remainder of page with silence */

	i = (NBPG / 2) - i;
	while (i-- > 0)
	    *sptr++ = 0;
    }

    fdcache(KERNELSPACE,audio_des->snd_extra_ptr,NBPG);
    _SYNC();

    return;
}
#endif

int
audio_test_mode(audio_des,atbptr)
    register struct audio_descriptor *audio_des;
    register struct audio_test_buffer *atbptr;
{
#ifdef __hp9000s700
    register struct audio2_reg *audio2_regs;
    register unsigned char *bufptr;
    register int i;

    if (audio_des->rcv_bufcount != 0 || audio_des->snd_bufcount != 0)
	return(EBUSY);

    audio2_regs = (struct audio2_reg *)audio_des->audio_regs;

    if (audio_des->flags & AUD_F_TESTMODE) {

	/* Turn Off Test Mode */

	audio_dma_stop(audio_des);
	sys_memfree(audio_des->test_buffer,MAX_TEST_BUFS * NBPG);
	audio_des->test_buffer = (unsigned char *)0;
	audio_des->flags &= ~AUD_F_TESTMODE;
    }
    else {

	/* Turn On Test Mode */

	if (audio2_regs->dstatus & (DSTAT_PC|DSTAT_RC) )
	    return(EBUSY);

	bufptr = (unsigned char *)sys_memall(MAX_TEST_BUFS * NBPG);
	if (bufptr == (unsigned char *)0)
	    return(ENOMEM);

	audio_des->test_buffer = bufptr;
	audio_des->flags |= AUD_F_TESTMODE;
	for (i = 0; i < MAX_TEST_BUFS; i++) {
	    atbptr->test_buffer_addr[i] = ltor(KERNELSPACE,bufptr);
	    bufptr += NBPG;
	}
    }

    return(0);
#endif
}

int
audio_test_rw_register(audio_des,arptr)
    register struct audio_descriptor *audio_des;
    register struct audio_regio *arptr;
{
#ifdef __hp9000s700
    register int i;
    register unsigned long *reg_ptr;
    register unsigned char *bufptr;

    i = arptr->audio_register;
    if (i < 0 || (i > AUDIO_MAX_REG && i != AUDIO_REG_DIAG))
	return(EINVAL);

    reg_ptr = (unsigned long *)audio_des->audio_regs;
    reg_ptr += i;

    if (arptr->writeflag) {
	if (   i == AUDIO_REG_PNXTADD || i == AUDIO_REG_PCURADD
	    || i == AUDIO_REG_RNXTADD || i == AUDIO_REG_RCURADD ) {

	    bufptr = audio_des->test_buffer;
	    for (i = 0; i < MAX_TEST_BUFS; i++) {
		if (arptr->value == ltor(KERNELSPACE,bufptr))
		    break;

		bufptr += NBPG;
	    }

	    if (i == MAX_TEST_BUFS)
		return(EINVAL);
	}
	*reg_ptr = arptr->value;
    }
    else
	arptr->value = *reg_ptr;

    return(0);
#endif
}

int
audio_test_rw_buffer(audio_des,abptr)
    register struct audio_descriptor *audio_des;
    register struct audio_bufio *abptr;
{
#ifdef __hp9000s700
    register int i;
    register unsigned char *bufptr;

    /* Validate physaddr */

    bufptr = audio_des->test_buffer;
    for (i = 0; i < MAX_TEST_BUFS; i++) {
	if (abptr->physaddr == ltor(KERNELSPACE,bufptr))
	    break;

	bufptr += NBPG;
    }

    if (i == MAX_TEST_BUFS)
	return(EINVAL);

    if (abptr->writeflag) {
	i = copyin(abptr->bufptr,bufptr,NBPG);
	if (i != 0)
	    return(i);
	fdcache(KERNELSPACE,bufptr,NBPG);
	_SYNC();
    }
    else {
	pdcache(KERNELSPACE,bufptr,NBPG);
	_SYNC();
	i = copyout(bufptr,abptr->bufptr,NBPG);
	if (i != 0)
	    return(i);
    }

    return(0);
#endif
}

struct audio_descriptor *
audio2_probe(dev)
    dev_t dev;
{
#ifdef __hp9000s300
    return((struct audio_descriptor *)0);
#else

    /* We don't probe if there is already an audio II device */

    if (audio_devices != (struct audio_descriptor *)0 && audio_devices->audio2flag != 0)
	return((struct audio_descriptor *)0);

    if (pdc_call(PDC_ADD_VALID, PDC_ADD_VALID_DEFAULT, AUDIO_2_HPA) != 0)
	return((struct audio_descriptor *)0);

    /* We have Audio II hardware */

    /* Map it */

    if (iovalidate((caddr_t)AUDIO_2_HPA,0,PDE_AR_KRW,AUDIO_2_HPA) == 0)
	return((struct audio_descriptor *)0);

    /* Initialize card */

    audio_probe_sc = m_selcode(dev);
    (void) audio_attach(AUDIO_2_CARD_ID,(struct isc_table_type *)0);

    return(audio_get_descriptor(m_selcode(dev)));
#endif
}

/* The following table maps alaw values to 16 bit linear values */

unsigned short alaw_tab[] = {
    0x1580, 0x1480, 0x1780, 0x1680, 0x1180, 0x1080, 0x1380, 0x1280,
    0x1d80, 0x1c80, 0x1f80, 0x1e80, 0x1980, 0x1880, 0x1b80, 0x1a80,
    0x0ac0, 0x0a40, 0x0bc0, 0x0b40, 0x08c0, 0x0840, 0x09c0, 0x0940,
    0x0ec0, 0x0e40, 0x0fc0, 0x0f40, 0x0cc0, 0x0c40, 0x0dc0, 0x0d40,
    0x5600, 0x5200, 0x5e00, 0x5a00, 0x4600, 0x4200, 0x4e00, 0x4a00,
    0x7600, 0x7200, 0x7e00, 0x7a00, 0x6600, 0x6200, 0x6e00, 0x6a00,
    0x2b00, 0x2900, 0x2f00, 0x2d00, 0x2300, 0x2100, 0x2700, 0x2500,
    0x3b00, 0x3900, 0x3f00, 0x3d00, 0x3300, 0x3100, 0x3700, 0x3500,
    0x0158, 0x0148, 0x0178, 0x0168, 0x0118, 0x0108, 0x0138, 0x0128,
    0x01d8, 0x01c8, 0x01f8, 0x01e8, 0x0198, 0x0188, 0x01b8, 0x01a8,
    0x0058, 0x0048, 0x0078, 0x0068, 0x0018, 0x0008, 0x0038, 0x0028,
    0x00d8, 0x00c8, 0x00f8, 0x00e8, 0x0098, 0x0088, 0x00b8, 0x00a8,
    0x0560, 0x0520, 0x05e0, 0x05a0, 0x0460, 0x0420, 0x04e0, 0x04a0,
    0x0760, 0x0720, 0x07e0, 0x07a0, 0x0660, 0x0620, 0x06e0, 0x06a0,
    0x02b0, 0x0290, 0x02f0, 0x02d0, 0x0230, 0x0210, 0x0270, 0x0250,
    0x03b0, 0x0390, 0x03f0, 0x03d0, 0x0330, 0x0310, 0x0370, 0x0350,
};

/* The following table maps ulaw values to 16 bit linear values */

unsigned short ulaw_tab[] = {
    0x7d7c, 0x797c, 0x757c, 0x717c, 0x6d7c, 0x697c, 0x657c, 0x617c,
    0x5d7c, 0x597c, 0x557c, 0x517c, 0x4d7c, 0x497c, 0x457c, 0x417c,
    0x3e7c, 0x3c7c, 0x3a7c, 0x387c, 0x367c, 0x347c, 0x327c, 0x307c,
    0x2e7c, 0x2c7c, 0x2a7c, 0x287c, 0x267c, 0x247c, 0x227c, 0x207c,
    0x1efc, 0x1dfc, 0x1cfc, 0x1bfc, 0x1afc, 0x19fc, 0x18fc, 0x17fc,
    0x16fc, 0x15fc, 0x14fc, 0x13fc, 0x12fc, 0x11fc, 0x10fc, 0x0ffc,
    0x0f3c, 0x0ebc, 0x0e3c, 0x0dbc, 0x0d3c, 0x0cbc, 0x0c3c, 0x0bbc,
    0x0b3c, 0x0abc, 0x0a3c, 0x09bc, 0x093c, 0x08bc, 0x083c, 0x07bc,
    0x075c, 0x071c, 0x06dc, 0x069c, 0x065c, 0x061c, 0x05dc, 0x059c,
    0x055c, 0x051c, 0x04dc, 0x049c, 0x045c, 0x041c, 0x03dc, 0x039c,
    0x036c, 0x034c, 0x032c, 0x030c, 0x02ec, 0x02cc, 0x02ac, 0x028c,
    0x026c, 0x024c, 0x022c, 0x020c, 0x01ec, 0x01cc, 0x01ac, 0x018c,
    0x0174, 0x0164, 0x0154, 0x0144, 0x0134, 0x0124, 0x0114, 0x0104,
    0x00f4, 0x00e4, 0x00d4, 0x00c4, 0x00b4, 0x00a4, 0x0094, 0x0084,
    0x0078, 0x0070, 0x0068, 0x0060, 0x0058, 0x0050, 0x0048, 0x0040,
    0x0038, 0x0030, 0x0028, 0x0020, 0x0018, 0x0010, 0x0008, 0x0000
};

/* The following table maps input and output gains, indexed 0 to 75,   */
/* representing -infinity to +14db, to the GX and GR gain coefficients */
/* required by the PSB2160 codec. Index 0 contains the coefficient     */
/* for -infinity. Indices 1 through 75 correspond to -60 db to +14 db  */
/* in 1 db increments.                                                 */

unsigned short audio_gain_tab[] = {
    0x0988, 0xF8B8, 0xF8B8, 0xF8B8, 0xF8B8, 0x099F, 0x099F, 0x099F,
    0x099F, 0x09AF, 0x09AF, 0x09AF, 0x09CF, 0x09CF, 0x09CF, 0xF8A9,
    0xF83A, 0xF83A, 0xF82B, 0xF82D, 0xF8A3, 0xF8B2, 0xF8A1, 0xE8AA,
    0xE84B, 0xE89E, 0xE8D3, 0xE891, 0xE8B1, 0xD8AA, 0xD8CB, 0xD8A6,
    0xD8B3, 0xD842, 0xD8B1, 0xC8AA, 0xC8BB, 0xC888, 0xC853, 0xC852,
    0xC8B1, 0xB8AA, 0xB8AB, 0xB896, 0xB892, 0xB842, 0xB8B1, 0xA8AA,
    0xA8BB, 0x199F, 0x195B, 0x29C1, 0x2923, 0x29AA, 0x392B, 0xF998,
    0xB988, 0x1AAC, 0x3AA1, 0xBAA1, 0xBB88, 0x8888, 0xD388, 0x5288,
    0xB1A1, 0x31A1, 0x1192, 0x11D0, 0x30C0, 0x2050, 0x1021, 0x1020,
    0x1000, 0x0001, 0x0010, 0x0000,
};

/* The following table maps the 16 possible beeper gains */
/* to tone generator gain coefficients.                  */

unsigned char beep_gain_tab[] = {
    0xa1, 0x22, 0x72, 0xa2, 0x23, 0x73, 0xa3, 0x24,
    0x74, 0xa4, 0x25, 0x75, 0xa5, 0x26, 0x66, 0x88
};

/* The following table maps the 16 possible beeper gains */
/* to transmit gain coefficients.                        */

unsigned short soundbeep_gain_tab[] = {
    0x8888, 0xbaa1, 0x1aac, 0xf998, 0x29aa, 0x29c1, 0x199f, 0xa8aa,
    0xb842, 0xb896, 0xb8aa, 0xc852, 0xc888, 0xc8aa, 0xd842, 0x0988
};

/* The following table maps monitor gain, indexed 0 to 55, represent-    */
/* ing -infinity to 0db, to the GZ gain coefficient required by the      */
/* PSB2160 codec. Index 0 contains the coefficient for -infinity.        */
/* Indices 1 through 55 correspond to -54 db to 0 db in 1 db increments. */

unsigned char monitor_gain_tab[] = {
    0x88, 0x97, 0x97, 0xA7, 0xA7, 0xA7, 0xC7, 0x67,
    0x37, 0x27, 0x17, 0xB6, 0xC6, 0x66, 0x36, 0x26,
    0x16, 0xB5, 0xC5, 0x65, 0x35, 0x25, 0x15, 0xB4,
    0xC4, 0x64, 0x34, 0x24, 0x14, 0xB3, 0xC3, 0x73,
    0x33, 0x23, 0x13, 0xB2, 0xC2, 0x72, 0x32, 0x22,
    0x12, 0xB1, 0xC1, 0x71, 0x31, 0x21, 0x11, 0xB0,
    0xC0, 0x01, 0x30, 0x20, 0x10, 0x10, 0x00, 0x00,
};

/* The following table converts the 10 bit frequency used by the */
/* Series 300 beeper to a square wave frequency coefficient that */
/* can be used by the PSB2160. There does not seem to be a good  */
/* formula that can be used to generate this data.               */

unsigned short beep_freq_tab[] = {
    0xA088, 0xA088, 0xA088, 0xA088, 0xA088, 0xA088, 0xA088, 0xA088,
    0xA088, 0x9088, 0x9088, 0x9088, 0xB188, 0xB188, 0xB188, 0xA188,
    0xA188, 0x2288, 0x2288, 0x2288, 0x9188, 0x9188, 0x9188, 0xC288,
    0xC288, 0xB288, 0xB288, 0xB288, 0xA288, 0xA288, 0x1399, 0x1399,
    0x1399, 0x2388, 0x2388, 0x2388, 0x2399, 0x2399, 0x3388, 0x3388,
    0x3388, 0x9288, 0x9288, 0xD388, 0xD388, 0xD388, 0xC388, 0xC388,
    0xC388, 0xB388, 0xB388, 0xA399, 0xA399, 0xA399, 0xA39A, 0xA39A,
    0xA388, 0xA388, 0xA388, 0x149A, 0x149A, 0x149A, 0x1499, 0x1499,
    0x2491, 0x2491, 0x2491, 0x2488, 0x2488, 0x2499, 0x2499, 0x2499,
    0x3491, 0x3491, 0x3491, 0x3488, 0x3488, 0x3499, 0x3499, 0x3499,
    0x4488, 0x4488, 0x4488, 0x9388, 0x9388, 0xE488, 0xE488, 0xE488,
    0xD488, 0xD488, 0xC488, 0xC488, 0xC488, 0xB499, 0xB499, 0xB499,
    0xB488, 0xB488, 0xB491, 0xB491, 0xB491, 0xB4A1, 0xB4A1, 0xA499,
    0xA499, 0xA499, 0xA49A, 0xA49A, 0xA49A, 0xA49B, 0xA49B, 0xA488,
    0xA488, 0xA488, 0x159B, 0x159B, 0x159A, 0x159A, 0x159A, 0x15AA,
    0x15AA, 0x15AA, 0x1599, 0x1599, 0x25A1, 0x25A1, 0x25A1, 0x2591,
    0x2591, 0x2592, 0x2592, 0x2592, 0x2593, 0x2593, 0x2593, 0x2588,
    0x2588, 0x259A, 0x259A, 0x259A, 0x2599, 0x2599, 0x35A1, 0x35A1,
    0x35A1, 0x3591, 0x3591, 0x3591, 0x3588, 0x3588, 0x359A, 0x359A,
    0x359A, 0x3599, 0x3599, 0x4591, 0x4591, 0x4591, 0x4588, 0x4588,
    0x4588, 0x4599, 0x4599, 0x5588, 0x5588, 0x5588, 0x9488, 0x9488,
    0xF588, 0xF588, 0xF588, 0xE588, 0xE588, 0xE588, 0xD588, 0xD588,
    0xC599, 0xC599, 0xC599, 0xC588, 0xC588, 0xC591, 0xC591, 0xC591,
    0xB599, 0xB599, 0xB599, 0xB59A, 0xB59A, 0xB59B, 0xB59B, 0xB59B,
    0xB588, 0xB588, 0xB588, 0xB592, 0xB592, 0xB591, 0xB591, 0xB591,
    0xB522, 0xB522, 0xB5A1, 0xB5A1, 0xB5A1, 0xA599, 0xA599, 0xA599,
    0xA5BA, 0xA5BA, 0xA5AA, 0xA5AA, 0xA5AA, 0xA59A, 0xA59A, 0x1692,
    0x1692, 0x1692, 0xA59B, 0xA59B, 0xA59B, 0xA59C, 0xA59C, 0xA588,
    0xA588, 0xA588, 0x169C, 0x169C, 0x169B, 0x169B, 0x169B, 0x16AB,
    0x16AB, 0x16AB, 0x169A, 0x169A, 0x162B, 0x162B, 0x162B, 0x16AA,
    0x16AA, 0x16BA, 0x16BA, 0x16BA, 0x1699, 0x1699, 0x1699, 0x26B1,
    0x26B1, 0x26A1, 0x26A1, 0x26A1, 0x2622, 0x2622, 0x2691, 0x2691,
    0x2691, 0x26A2, 0x26A2, 0x26A2, 0xA5B1, 0xA5B1, 0x2692, 0x2692,
    0x2692, 0x2693, 0x2693, 0x2688, 0x2688, 0x2688, 0x269B, 0x269B,
    0x269B, 0x26AB, 0x26AB, 0x269A, 0x269A, 0x269A, 0x26AA, 0x26AA,
    0x2699, 0x2699, 0x2699, 0x36B1, 0x36B1, 0x36B1, 0x36A1, 0x36A1,
    0x3691, 0x3691, 0x3691, 0x36A2, 0x36A2, 0x3692, 0x3692, 0x3692,
    0x3688, 0x3688, 0x3688, 0x369B, 0x369B, 0x369A, 0x369A, 0x369A,
    0x36AA, 0x36AA, 0x3699, 0x3699, 0x3699, 0x4691, 0x4691, 0x4691,
    0x4692, 0x4692, 0x4688, 0x4688, 0x4688, 0x469A, 0x469A, 0x469A,
    0x4699, 0x4699, 0x5691, 0x5691, 0x5691, 0x5688, 0x5688, 0x5699,
    0x5699, 0x5699, 0x6688, 0x6688, 0x6688, 0x9588, 0x9588, 0x0788,
    0x0788, 0x0788, 0xF688, 0xF688, 0xE688, 0xE688, 0xE688, 0xD699,
    0xD699, 0xD699, 0xD688, 0xD688, 0xD691, 0xD691, 0xD691, 0xC699,
    0xC699, 0xC69A, 0xC69A, 0xC69A, 0xC688, 0xC688, 0xC688, 0xC692,
    0xC692, 0xC691, 0xC691, 0xC691, 0xB61A, 0xB61A, 0xC6A1, 0xC6A1,
    0xC6A1, 0xB699, 0xB699, 0xB699, 0xB6AA, 0xB6AA, 0xB69A, 0xB69A,
    0xB69A, 0xB69B, 0xB69B, 0xB69C, 0xB69C, 0xB69C, 0xB688, 0xB688,
    0xB688, 0xB693, 0xB693, 0xB692, 0xB692, 0xB692, 0x17B1, 0x17B1,
    0xB6A2, 0xB6A2, 0xB6A2, 0xB691, 0xB691, 0xB691, 0x17A1, 0x17A1,
    0xB622, 0xB622, 0xB622, 0xB6A1, 0xB6A1, 0xB6A1, 0xB6A1, 0xB6B1,
    0xB6B1, 0xB6B1, 0xB6B1, 0xA699, 0xA699, 0xA6CA, 0xA6CA, 0xA6CA,
    0xA6BA, 0xA6BA, 0xA6AA, 0xA6AA, 0xA6AA, 0x17A2, 0x17A2, 0x17A2,
    0xA62B, 0xA62B, 0xA69A, 0xA69A, 0xA69A, 0x1792, 0x1792, 0xA6AB,
    0xA6AB, 0xA6AB, 0xA62C, 0xA62C, 0xA62C, 0xA69B, 0xA69B, 0xA69C,
    0xA69C, 0xA69C, 0xA69D, 0xA69D, 0xA69D, 0xA688, 0xA688, 0x1788,
    0x1788, 0x1788, 0x179C, 0x179C, 0x17AC, 0x17AC, 0x17AC, 0x179B,
    0x179B, 0x179B, 0x172C, 0x172C, 0x17AB, 0x17AB, 0x17AB, 0x17BB,
    0x17BB, 0x179A, 0x179A, 0x179A, 0x173B, 0x173B, 0x173B, 0x172B,
    0x172B, 0x172B, 0x17AA, 0x17AA, 0x17AA, 0x17AA, 0x17AA, 0x17AA,
    0x17BA, 0x17BA, 0x17BA, 0x17BA, 0x17CA, 0x17CA, 0x1799, 0x1799,
    0x1799, 0x27C1, 0x27C1, 0x27B1, 0x27B1, 0x27B1, 0x27B1, 0xA622,
    0xA622, 0xA622, 0xA622, 0x27A1, 0x27A1, 0x27A1, 0x27A1, 0x2722,
    0x2722, 0x2722, 0x2722, 0x2732, 0x2732, 0x2732, 0x2791, 0x2791,
    0x27B2, 0x27B2, 0x27B2, 0x27A2, 0x27A2, 0x27A2, 0x27A2, 0xA6B1,
    0xA6B1, 0xA6B1, 0xA6B1, 0x2792, 0x2792, 0x27A3, 0x27A3, 0x27A3,
    0x2793, 0x2793, 0x27A4, 0x27A4, 0x27A4, 0x9699, 0x9699, 0x9699,
    0x2788, 0x2788, 0x279C, 0x279C, 0x279C, 0x27AC, 0x27AC, 0x279B,
    0x279B, 0x279B, 0x27AB, 0x27AB, 0x27AB, 0x279A, 0x279A, 0x273B,
    0x273B, 0x273B, 0x272B, 0x272B, 0x27AA, 0x27AA, 0x27AA, 0x27BA,
    0x27BA, 0x27BA, 0x96AA, 0x96AA, 0x2799, 0x2799, 0x2799, 0x37B1,
    0x37B1, 0x37B1, 0x37B1, 0x37A1, 0x37A1, 0x37A1, 0x37A1, 0x3722,
    0x3722, 0x3732, 0x3732, 0x3732, 0x3791, 0x3791, 0x3791, 0x37A2,
    0x37A2, 0x3723, 0x3723, 0x3723, 0x3792, 0x3792, 0x3793, 0x3793,
    0x3793, 0x969A, 0x969A, 0x969A, 0x3788, 0x3788, 0x379B, 0x379B,
    0x379B, 0x37AB, 0x37AB, 0x379A, 0x379A, 0x379A, 0x372B, 0x372B,
    0x372B, 0x37AA, 0x37AA, 0x3799, 0x3799, 0x3799, 0x47B1, 0x47B1,
    0x47A1, 0x47A1, 0x47A1, 0x4722, 0x4722, 0x4722, 0x4791, 0x4791,
    0x47A2, 0x47A2, 0x47A2, 0x969B, 0x969B, 0x4788, 0x4788, 0x4788,
    0x479B, 0x479B, 0x479B, 0x479A, 0x479A, 0x47AA, 0x47AA, 0x47AA,
    0x4799, 0x4799, 0x57A1, 0x57A1, 0x57A1, 0x5791, 0x5791, 0x5791,
    0x969C, 0x969C, 0x5788, 0x5788, 0x5788, 0x579A, 0x579A, 0x5799,
    0x5799, 0x5799, 0x969D, 0x969D, 0x969D, 0x6788, 0x6788, 0x6799,
    0x6799, 0x6799, 0x9688, 0x9688, 0x9688, 0x9688, 0xF798, 0xF798,
    0xF798, 0xF798, 0xF788, 0xF788, 0xE799, 0xE799, 0xE799, 0xE788,
    0xE788, 0xE791, 0xE791, 0xE791, 0xD799, 0xD799, 0xD799, 0xD79A,
    0xD79A, 0xD788, 0xD788, 0xD788, 0xD792, 0xD792, 0xD792, 0xD791,
    0xD791, 0xD7A1, 0xD7A1, 0xD7A1, 0xC799, 0xC799, 0xC7AA, 0xC7AA,
    0xC7AA, 0xC79A, 0xC79A, 0xC79A, 0xC79B, 0xC79B, 0xC79B, 0xC788,
    0xC788, 0xC788, 0xC788, 0xC793, 0xC793, 0xC793, 0xC792, 0xC792,
    0xC792, 0xC7A2, 0xC7A2, 0xC791, 0xC791, 0xC791, 0xB71A, 0xB71A,
    0xB71A, 0xB71A, 0xC7A1, 0xC7A1, 0xC7A1, 0xC7A1, 0xC7B1, 0xC7B1,
    0xB799, 0xB799, 0xB799, 0xB7BA, 0xB7BA, 0xB7AA, 0xB7AA, 0xB7AA,
    0xB7AA, 0xB71B, 0xB71B, 0xB71B, 0xB71B, 0xB79A, 0xB79A, 0xB79A,
    0xB7AB, 0xB7AB, 0xB79B, 0xB79B, 0xB79B, 0xB79B, 0xB79C, 0xB79C,
    0xB79C, 0xB79C, 0xB788, 0xB788, 0xB788, 0xB794, 0xB794, 0xB794,
    0xB794, 0xB793, 0xB793, 0xB793, 0xB793, 0xB7A3, 0xB7A3, 0xB792,
    0xB792, 0xB792, 0xB792, 0xB723, 0xB723, 0xB723, 0xB723, 0xB7A2,
    0xB7A2, 0xB7A2, 0xB7A2, 0xB7A2, 0xB791, 0xB791, 0xB791, 0xB791,
    0xB791, 0xA71A, 0xA71A, 0xA71A, 0xA71A, 0xB722, 0xB722, 0xB722,
    0xB722, 0xB722, 0xB722, 0xB722, 0xB7A1, 0xB7A1, 0xB7A1, 0xB7A1,
    0xB7A1, 0xA72A, 0xA72A, 0xA72A, 0xA72A, 0xB7B1, 0xB7B1, 0xB7B1,
    0xB7B1, 0xB7C1, 0xB7C1, 0xB7C1, 0xB7C1, 0xA799, 0xA799, 0xA799,
    0xA799, 0xA7CA, 0xA7CA, 0xA7CA, 0xA7CA, 0xA7BA, 0xA7BA, 0xA7BA,
    0xA7BA, 0xA7BA, 0xA7BA, 0xA7AA, 0xA7AA, 0xA7AA, 0xA7AA, 0xA7AA,
    0xA71B, 0xA71B, 0xA71B, 0xA71B, 0xA71B, 0xA71B, 0xA71B, 0xA72B,
    0xA72B, 0xA72B, 0xA72B, 0xA72B, 0xA72B, 0xA79A, 0xA79A, 0xA79A,
    0xA79A, 0xA7BB, 0xA7BB, 0xA7BB, 0xA7BB, 0xA7AB, 0xA7AB, 0xA7AB,
    0xA7AB, 0xA71C, 0xA71C, 0xA71C, 0xA71C, 0xA71C, 0xA79B, 0xA79B,
    0xA79B, 0xA79B, 0xA79B, 0xA7AC, 0xA7AC, 0xA7AC, 0xA7AC, 0xA79C,
    0xA79C, 0xA79C, 0xA79C, 0xA79D, 0xA79D, 0xA79D, 0xA79D, 0xA79D,
    0xA79D, 0xA788, 0xA788, 0xA788, 0xA788, 0xA788, 0xA795, 0xA795,
    0xA795, 0xA795, 0xA794, 0xA794, 0xA794, 0xA794, 0xA7A4, 0xA7A4,
    0xA7A4, 0xA7A4, 0xA7A4, 0xA793, 0xA793, 0xA793, 0xA793, 0xA793,
    0xA724, 0xA724, 0xA724, 0xA724, 0xA724, 0xA7A3, 0xA7A3, 0xA7A3,
    0xA7A3, 0xA7A3, 0xA7B3, 0xA7B3, 0xA7B3, 0xA7B3, 0xA7B3, 0xA7B3,
    0xA792, 0xA792, 0xA792, 0xA792, 0xA792, 0xA733, 0xA733, 0xA733,
    0xA733, 0xA733, 0xA723, 0xA723, 0xA723, 0xA723, 0xA723, 0xA723,
    0xA723, 0xA723, 0xA7A2, 0xA7A2, 0xA7A2, 0xA7A2, 0xA7A2, 0xA7A2,
    0xA7A2, 0xA7A2, 0xA7A2, 0xA7A2, 0xA7B2, 0xA7B2, 0xA7B2, 0xA7B2,
    0xA7B2, 0xA7B2, 0xA7B2, 0xA7B2, 0xA7C2, 0xA7C2, 0xA7C2, 0xA7C2,
    0xA7C2, 0xA791, 0xA791, 0xA791, 0xA791, 0xA791, 0x971A, 0x971A,
    0x971A, 0x971A, 0x971A, 0x971A, 0xA732, 0xA732, 0xA732, 0xA732,
    0xA732, 0xA732, 0xA732, 0xA732, 0xA732, 0xA722, 0xA722, 0xA722,
    0xA722, 0xA722, 0xA722, 0xA722, 0xA722, 0xA722, 0xA722, 0xA722,
    0xA722, 0xA722, 0xA722, 0xA722, 0xA722, 0xA722, 0xA722, 0xA7A1,
    0xA7A1, 0xA7A1, 0xA7A1, 0xA7A1, 0xA7A1, 0xA7A1, 0xA7A1, 0xA7A1,
    0xA7A1, 0xA7A1, 0xA7A1, 0xA7A1, 0xA7A1, 0x972A, 0x972A, 0x972A,
};

/*
 * The following table contains the supported set of sampling rates
 * for the audio II interface.
 */

int sample_rate_tab[] = {
    8000,   11025,  16000,  22050,  32000, 44100, 48000,
};

/*
 * The following table contains the register values for the above
 * set of sample rates, in a one to one correspondence.
 */

#ifdef __hp9000s700
unsigned int sample_rate_cftab[] = {
    CNTL_RATE_8000,   CNTL_RATE_11025,  CNTL_RATE_16000, CNTL_RATE_22050,
    CNTL_RATE_32000,  CNTL_RATE_44100, CNTL_RATE_48000,
};
#endif

/*
 * The following table contains samples of one full sine wave cycle.
 * It is used to generate sine waves to emulate a beeper.
 */

short sine_wave_table[SINE_TABLE_SIZE] = {
	    0,   3212,   6393,   9512,  12539,  15446,  18204,  20787,
	23170,  25329,  27245,  28898,  30273,  31356,  32137,  32609,
	32767,  32609,  32137,  31356,  30273,  28898,  27245,  25329,
	23170,  20787,  18204,  15446,  12539,   9512,   6393,   3212,
	    0,  -3212,  -6393,  -9512, -12539, -15446, -18204, -20787,
       -23170, -25329, -27245, -28898, -30273, -31356, -32137, -32609,
       -32767, -32609, -32137, -31356, -30273, -28898, -27245, -25329,
       -23170, -20787, -18204, -15446, -12539,  -9512,  -6393,  -3212,
	    0,
};
