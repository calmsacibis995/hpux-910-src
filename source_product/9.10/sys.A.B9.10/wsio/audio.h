/*
 * @(#)audio.h: $Revision: 1.6.83.4 $ $Date: 93/12/06 16:42:20 $
 * $Locker:  $
 */

#ifndef _SYS_AUDIO_INCLUDED /* allow multiple inclusions */
#define _SYS_AUDIO_INCLUDED


#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

#ifdef __hp9000s700
#define CRIT()      spl5()
#define UNCRIT(x)    (splx(x))
#endif

/* Bit mask arguments for AUDIO_SET_OUTPUT */

#define AUDIO_OUT_NONE        0x00
#define AUDIO_OUT_INTERNAL    0x01      /* Internal Speaker */
#define AUDIO_OUT_EXTERNAL    0x02      /* headphones */
#define AUDIO_OUT_LINE        0x04      /* Line Level Out (Audio II only) */

#define AUDIO_OUT_SPEAKER     AUDIO_OUT_INTERNAL
#define AUDIO_OUT_HEADPHONE   AUDIO_OUT_EXTERNAL

/* Bit mask arguments for AUDIO_SET_INPUT */

#define AUDIO_IN_NONE  0x00  /* Not supported for Audio I or II - Future */
#define AUDIO_IN_MIKE  0x01  /* Microphone input */
#define AUDIO_IN_LINE  0x02  /* Line level input (Audio II only) */

/* Arguments for AUDIO_SET_FORMAT */

#define AUDIO_FORMAT_ULAW        0
#define AUDIO_FORMAT_ALAW        1
#define AUDIO_FORMAT_LINEAR16BIT 2

/* Valid audio id's returned by AUDIO_DESCRIBE */

#define AUDIO_ID_PSB2160   1
#define AUDIO_ID_CS4215    2

/* Structure for AUDIO_DESCRIBE */

struct audio_describe {
    unsigned int audio_id;
    unsigned int nrates;
    unsigned int flags;
    unsigned int max_bits_per_sample;
    unsigned int nchannels;
    int min_receive_gain;
    int max_receive_gain;
    int min_transmit_gain;
    int max_transmit_gain;
    int min_monitor_gain;
    int max_monitor_gain;
    int sample_rate[8];
    int spare[12];
};

/* Flags for AUDIO_DESCRIBE flags field */

#define AD_F_NOBEEPER 0x00000001  /* SPU does not have a separate beeper */


/* Structure for AUDIO_SET_LIMITS and AUDIO_GET_LIMITS */

struct audio_limits {
    unsigned int max_receive_buffer_size;
    unsigned int max_transmit_buffer_size;
    unsigned int spare[4];
};

/* Defines that may be useful for AUDIO_SET_GAINS */

#define AUDIO_OFF_GAIN          -128    /* -infinite gain */
#define AUDIO_MAX_GAIN           127    /* maximum gain provided by */
					/* hardware.                */

    /* Various values for channel_mask */

#define AUDIO_CHANNEL_0     0x0001
#define AUDIO_CHANNEL_1     0x0002
#define AUDIO_CHANNEL_LEFT  AUDIO_CHANNEL_0
#define AUDIO_CHANNEL_RIGHT AUDIO_CHANNEL_1

/* Structure for AUDIO_SET_GAINS and AUDIO_GET_GAINS */

/* Old structure for backwards compatibility */

struct audio_gains {
    int receive_gain;
    int transmit_gain;
    int monitor_gain;
    int spare[4];
};

struct channel_gain {
    int receive_gain;
    int transmit_gain;
    int monitor_gain;
};

struct audio_gain {
    struct channel_gain cgain[2];
    int channel_mask;
};

/* Structure for AUDIO_SET_SEL_THRESHOLD and AUDIO_GET_SEL_THRESHOLD */

struct audio_select_thresholds {
    unsigned int read_threshold;
    unsigned int write_threshold;
    unsigned int spare[4];
};

/* Defines for audio status */

#define AUDIO_DONE      0
#define AUDIO_BUSY      1
#define AUDIO_PAUSED    2

/* Structure for AUDIO_GET_STATUS */

struct audio_status {
    unsigned int receive_status;
    unsigned int transmit_status;
    unsigned int receive_buffer_count;
    unsigned int transmit_buffer_count;
    unsigned int receive_overflow_count;
    unsigned int transmit_underflow_count;
    unsigned int spare[4];
};

/* Defines for AUDIO_SET_BEEP */

#define AUDIO_BEEP_TONE  0
#define AUDIO_BEEP_ULAW  1

/* Structure for AUDIO_SET_BEEP and AUDIO_GET_BEEP */

struct audio_beep_type {
    unsigned int type;
    unsigned int datalen;      /* Not used for AUDIO_BEEP_TONE */
    unsigned char *data;       /* Not used for AUDIO_BEEP_TONE */
    unsigned int returnlen;    /* Not used for AUDIO_BEEP_TONE */
    unsigned int repeat_count; /* Not used for AUDIO_BEEP_TONE */
    unsigned int sample_rate;  /* Not used for AUDIO_BEEP_TONE - Audio II only */
    unsigned int nchannels;    /* Not used for AUDIO_BEEP_TONE - Audio II only */
    unsigned int spare;
};

/* Defines for AUDIO_RESET ioctl */

#define RESET_RX_BUF  0x01
#define RESET_TX_BUF  0x02
#define RESET_RX_OVF  0x04
#define RESET_TX_UNF  0x08
#define RESET_OVRANGE 0x10 /* Audio II only */

/* Defines for AUDIO_PAUSE & AUDIO_RESUME */

#define AUDIO_RECEIVE  0x01
#define AUDIO_TRANSMIT 0x02

/* Device specific structure for psb2160 configuration */

struct psb2160conf {
    unsigned short rx_gain;     /* Receive gain  */
    unsigned short tx_gain;     /* Transmit gain */
    unsigned short tone_time1;  /* Tone Generator 1 time */
    unsigned short tone_time2;  /* Tone Generator 2 time */
    unsigned short tone_time3;  /* Tone Generator 3 time */
    unsigned short tone_freq1;  /* Tone Generator 1 frequency */
    unsigned short tone_freq2;  /* Tone Generator 2 frequency */
    unsigned short tone_freq3;  /* Tone Generator 3 frequency */
    unsigned char tone_gain1;   /* Tone Generator 1 gain */
    unsigned char tone_gain2;   /* Tone Generator 2 gain */
    unsigned char tone_gain3;   /* Tone Generator 3 gain */
    unsigned char monitor_gain; /* monitor gain (side-tone) */
    unsigned char dtmf_high;    /* DTMF generator coefficient */
    unsigned char dtmf_low;     /* DTMF generator coefficient */
    unsigned char conf1;        /* Configuration register 1 */
    unsigned char conf2;        /* Configuration register 2 */
    unsigned char conf3;        /* Configuration register 3 */
    unsigned char conf4;        /* Configuration register 4 */
    unsigned char fr[10];       /* Receive  Freq. equalizer coeff. */
    unsigned char fx[10];       /* Transmit Freq. equalizer coeff. */
};

/* Device specific structure for cs4215 configuration */

struct cs4215conf {
    unsigned long control;
    unsigned long dmastatus;    /* Read Only */
    unsigned long gainctl;
    unsigned long over_range;
    unsigned long pio;
};

#define AUDIO_CONF_PAD 126

/* Structure for AUDIO_RAW_SET_PARAMS and AUDIO_RAW_GET_PARAMS */

struct raw_audio_config {
    union {
	struct psb2160conf psb2160_conf;
	struct cs4215conf  cs4215_conf;
	unsigned char pad[AUDIO_CONF_PAD];
    } audio_conf_union;
};

/* Struct for AUDIO_METER */

    /* Old structure for backwards compatibility */

struct audio_meter {
    unsigned long   receive_nsamples;
    unsigned long   spare1[3];
    unsigned long   receive_sample_sum;
    unsigned long   spare2[3];              /* Future channels? */
    unsigned long   receive_sample_peak;
    unsigned long   spare3[3];              /* Future channels? */
    unsigned long   transmit_nsamples;
    unsigned long   spare4[3];              /* Future channels? */
    unsigned long   transmit_sample_sum;
    unsigned long   spare5[3];              /* Future channels? */
    unsigned long   transmit_sample_peak;
    unsigned long   spare6[5];              /* Future channels? + extra */
    unsigned short  receive_meter_state;
    unsigned short  spare7[3];
    unsigned short  transmit_meter_state;
    unsigned short  spare8[3];
};

struct audio_datainfo {
    unsigned long   receive_nsamples;
    unsigned long   receive_overrange_flag;
    unsigned long   spare1[2];
    unsigned long   receive_sample_sum[2];
    unsigned long   spare2[2];              /* Future channels? */
    unsigned long   receive_sample_peak[2];
    unsigned long   spare3[2];              /* Future channels? */
    unsigned long   transmit_nsamples;
    unsigned long   spare4[3];
    unsigned long   transmit_sample_sum[2];
    unsigned long   spare5[2];              /* Future channels? */
    unsigned long   transmit_sample_peak[2];
    unsigned long   spare6[4];              /* Future channels? + extra */
    unsigned short  receive_meter_state;
    unsigned short  spare7[3];
    unsigned short  transmit_meter_state;
    unsigned short  spare8[3];
};

#define METER_OFF 0
#define METER_ON  1

struct audio_meter_ctl {
    unsigned  short receive_meter_state;
    unsigned  short spare2[3];
    unsigned  short transmit_meter_state;
    unsigned  short spare1[7];
};

/* Defines for TEST ioctl's, do not use for normal applications */

#define AUDIO_REG_ID       0
#define AUDIO_REG_RESET    1
#define AUDIO_REG_CNTL     2
#define AUDIO_REG_GAINCTL  3
#define AUDIO_REG_PNXTADD  4
#define AUDIO_REG_PCURADD  5
#define AUDIO_REG_RNXTADD  6
#define AUDIO_REG_RCURADD  7
#define AUDIO_REG_DSTATUS  8
#define AUDIO_REG_OVRANGE  9
#define AUDIO_REG_PIO     10
#define AUDIO_MAX_REG     10
#define AUDIO_REG_DIAG    15  /* Special case */

#define MAX_TEST_BUFS 4

struct audio_test_buffer {
    unsigned long test_buffer_addr[MAX_TEST_BUFS];
    int spare[12 - MAX_TEST_BUFS];
};

struct audio_regio {
    int writeflag;
    int audio_register;
    unsigned long value;
    int spare[8];
};

struct audio_bufio {
    int writeflag;
    unsigned long physaddr;
    char *bufptr;
    int spare[8];
};

/* Defines for ioctl's */

#define AUDIO_DESCRIBE          _IOR('A',1,struct audio_describe)
#define AUDIO_SET_LIMITS        _IOW('A',2,struct audio_limits)
#define AUDIO_GET_LIMITS        _IOR('A',3,struct audio_limits)
#define AUDIO_SET_RXBUFSIZE     _IO('A',4)
#define AUDIO_GET_RXBUFSIZE     _IOR('A',5,int)
#define AUDIO_SET_TXBUFSIZE     _IO('A',6)
#define AUDIO_GET_TXBUFSIZE     _IOR('A',7,int)
#define AUDIO_SET_OUTPUT        _IO('A',8)
#define AUDIO_GET_OUTPUT        _IOR('A',9,int)
#define AUDIO_SET_GAINS         _IOW('A',10,struct audio_gain)
#define AUDIO_GET_GAINS         _IOR('A',11,struct audio_gain)
#define AUDIO_SET_DATA_FORMAT   _IO('A',12)
#define AUDIO_GET_DATA_FORMAT   _IOR('A',13,int)
#define AUDIO_SET_SEL_THRESHOLD _IOW('A',14,struct audio_select_thresholds)
#define AUDIO_GET_SEL_THRESHOLD _IOR('A',15,struct audio_select_thresholds)
#define AUDIO_GET_STATUS        _IOR('A',16,struct audio_status)
#define AUDIO_RESET             _IO('A',17)
#define AUDIO_PAUSE             _IO('A',18)
#define AUDIO_RESUME            _IO('A',19)
#define AUDIO_DRAIN             _IO('A',20)
#define AUDIO_RAW_SET_PARAMS    _IOW('A',21,struct raw_audio_config)
#define AUDIO_RAW_GET_PARAMS    _IOR('A',22,struct raw_audio_config)
#define AUDIO_METER             _IOR('A',23,struct audio_datainfo)
#define AUDIO_METER_CTL         _IOW('A',24,struct audio_meter_ctl)
#define AUDIO_SET_SAMPLE_RATE   _IO('A',25)
#define AUDIO_GET_SAMPLE_RATE   _IOR('A',26,int)
#define AUDIO_SET_CHANNELS      _IO('A',27)
#define AUDIO_GET_CHANNELS      _IOR('A',28,int)
#define AUDIO_SET_INPUT         _IO('A',29)
#define AUDIO_GET_INPUT         _IOR('A',30,int)

#define AUDIO_SET_BEEP          _IOW('A',40,struct audio_beep_type)
#define AUDIO_GET_BEEP          _IOWR('A',41,struct audio_beep_type)

#define AUDIO_TEST_MODE         _IOR('A',80,struct audio_test_buffer)
#define AUDIO_TEST_RW_REGISTER  _IOWR('A',81,struct audio_regio)
#define AUDIO_TEST_RW_BUFFER    _IOWR('A',82,struct audio_bufio)
#define AUDIO_TEST_INT          _IO('A',83)

/* Defines for bits in psb2160 configuration registers */

#define CR1_TEST_MODE_MASK 0x07
#define CR1_TM_ALS 0x01
#define CR1_TM_ALM 0x02
#define CR1_TM_BYP 0x03
#define CR1_TM_IDR 0x04
#define CR1_TM_DLS 0x05
#define CR1_TM_DLM 0x06
#define CR1_TM_DLP 0x07
#define CR1_GX  0x08
#define CR1_FR  0x10
#define CR1_FX  0x20
#define CR1_GZ  0x40
#define CR1_GR  0x80

#define CR2_EFC   0x01
#define CR2_TR    0x02
#define CR2_AM    0x04
#define CR2_ELS   0x08
#define CR2_SA    0x10
#define CR2_SB    0x20
#define CR2_SC    0x40
#define CR2_SD    0x80
#define CR2_SABCD 0xf0

#define CR3_MIKE_GAIN_MASK 0xe0
#define CR3_OUTPUT_MASK    0x1c
#define CR3_OPMODE_MASK    0x03

#define CR3_OM_NORMAL      0x00
#define CR3_OM_MIXED       0x01
#define CR3_OM_LINEAR      0x02
#define CR3_OM_RESERVE     0x03

#define CR3_OUT_POR        0x00
#define CR3_OUT_RDY        0x04
#define CR3_OUT_LH1        0x08
#define CR3_OUT_LH2        0x0c
#define CR3_OUT_LH3        0x10
#define CR3_OUT_HFS        0x14
#define CR3_OUT_MUT        0x18
#define CR3_OUT_RES        0x1c

#define CR3_MG_52          0x00
#define CR3_MG_46          0x20
#define CR3_MG_40          0x40
#define CR3_MG_34          0x60
#define CR3_MG_28          0x80
#define CR3_MG_22          0xa0
#define CR3_MG_16          0xc0
#define CR3_MG_XIN         0xe0

#define CR4_ULAW           0x01
#define CR4_PM             0x02
#define CR4_BM             0x04
#define CR4_TM             0x08
#define CR4_BT             0x10
#define CR4_TG             0x20
#define CR4_DTMF           0x40
#define CR4_DHF            0x80

/* Defines for bits in cs4215 configuration registers */

#define CNTL_STATUS        0x80000000

#define CNTL_LS_NONE       0x00000000
#define CNTL_LS_ASIC       0x00000100
#define CNTL_LS_DIG        0x00000200
#define CNTL_LS_ANA        0x00000300

#define CNTL_DF_LINEAR     0x00000000
#define CNTL_DF_ULAW       0x00000040
#define CNTL_DF_ALAW       0x00000080
#define CNTL_DF_MASK       0x000000c0

#define CNTL_STEREO        0x00000020

#define CNTL_RATE_5512     0x00000010
#define CNTL_RATE_6615     0x00000017
#define CNTL_RATE_8000     0x00000008
#define CNTL_RATE_9600     0x0000000f
#define CNTL_RATE_11025    0x00000011
#define CNTL_RATE_16000    0x00000009
#define CNTL_RATE_18900    0x00000012
#define CNTL_RATE_22050    0x00000013
#define CNTL_RATE_27428    0x0000000a
#define CNTL_RATE_32000    0x0000000b
#define CNTL_RATE_33075    0x00000016
#define CNTL_RATE_37800    0x00000014
#define CNTL_RATE_44100    0x00000015
#define CNTL_RATE_48000    0x0000000e
#define CNTL_RATE_MASK     0x0000001f

#define DSTAT_IE           0x80000000
#define DSTAT_PN           0x00000200
#define DSTAT_PC           0x00000100
#define DSTAT_RN           0x00000002
#define DSTAT_RC           0x00000001

#define GAIN_HE            0x08000000
#define GAIN_LE            0x04000000
#define GAIN_SE            0x02000000
#define GAIN_IS            0x01000000

#define GAIN_MA_MASK       0x00f00000
#define GAIN_MA_SHIFT              20

#define GAIN_LI_MASK       0x000f0000
#define GAIN_LI_SHIFT              16

#define GAIN_RI_MASK       0x0000f000
#define GAIN_RI_SHIFT              12

#define GAIN_LO_MASK       0x00000fc0
#define GAIN_LO_SHIFT               6

#define GAIN_RO_MASK       0x0000003f

#define OVR_BIT            0x00000001

#define PIO_PO_MASK               0x3
#define PIO_PO_SHIFT                2
#define PIO_PI_MASK               0x1

#ifdef _KERNEL

/* macro to determine whether this is a control only device */

#ifdef __hp9000s300
#define IS_CTL(x) (x & 0x1000)
#else
#define IS_CTL(x) (x & 0x0100)
#endif

#define INFIFO_SIZE  128
#define OUTFIFO_SIZE 128
#define CTLFIFO_SIZE  16

#define MIN_BUFFER_SIZE_1       2048
#define MIN_BUFFER_SIZE_2       8192
#define DEF_MAX_BUFFER_SIZE_1  65536
#define DEF_MAX_BUFFER_SIZE_2 262144
#define DEF_SEL_THRESHOLD       1024

#define MIN_GAIN_1     -60
#define MAX_GAIN_1      14
#define MIN_MON_GAIN_1 -54
#define MAX_MON_GAIN_1   0

#define MIN_RX_GAIN_2    0
#define MAX_RX_GAIN_2   22
#define MIN_TX_GAIN_2  -84
#define MAX_TX_GAIN_2    9
#define MIN_MON_GAIN_2 -84
#define MAX_MON_GAIN_2   0

#define ALAW_QUIET  0x55
#define ULAW_QUIET  0xff

#define EXTRA_BUFSIZE 8192
#define AUDIO_SWAP_THRESHOLD 0xE80

#define NSRATES 7

#ifdef __hp9000s300
struct audio1_reg {
    volatile unsigned char filler1;
    volatile unsigned char idreset;
    volatile unsigned char filler2;
    volatile unsigned char interrupt;
    volatile unsigned char filler3;
    volatile unsigned char fifointctl;
    volatile unsigned char filler4;
    volatile unsigned char fifostatus;
    volatile unsigned char filler5;
    volatile unsigned char inoutfifo;
    volatile unsigned char filler6;
    volatile unsigned char ctlfifo;
};
#else
struct audio1_reg {
    volatile unsigned char filler0;
    volatile unsigned char idreset;    /* Sound ID and Reset Register (r/w)    */
    volatile unsigned char filler1;
    volatile unsigned char filler2;
    volatile unsigned char filler3;
    volatile unsigned char interrupt;  /* Status and Control Register (r/w)    */
    volatile unsigned char filler4;
    volatile unsigned char filler5;
    volatile unsigned char filler6;
    volatile unsigned char fifointctl; /* FIFO Interrupt Register (r/w)        */
    volatile unsigned char filler7;
    volatile unsigned char filler8;
    volatile unsigned char filler9;
    volatile unsigned char fifostatus; /* FIFO Status Register (read only)     */
    volatile unsigned char fillera;
    volatile unsigned char fillerb;
    volatile unsigned char fillerc;
    volatile unsigned char inoutfifo;  /* Sound Data Port (r/w)                */
    volatile unsigned char fillerd;
    volatile unsigned char fillere;
    volatile unsigned char fillerf;
    volatile unsigned char ctlfifo;    /* Sound Control Port (write only)      */
};
#endif

struct audio2_reg {
    volatile unsigned long id;         /* Sound ID (read only) */
    volatile unsigned long reset;      /* Reset Register (r/w) */
    volatile unsigned long cntl;       /* Control Register (r/w) */
    volatile unsigned long gainctl;    /* Gain Control (r/w) */
    volatile unsigned long pnxtadd;    /* Playback Next Address (r/w) */
    volatile unsigned long pcuradd;    /* Playback Current Address (read only) */
    volatile unsigned long rnxtadd;    /* Record Next Address (r/w) */
    volatile unsigned long rcuradd;    /* Record Current Address (read only) */
    volatile unsigned long dstatus;    /* DMA status (read only) */
    volatile unsigned long ovrange;    /* Over Range (r/w) */
    volatile unsigned long pio;        /* PIO register (r/w) */
    unsigned long filler[4];
    volatile unsigned long diag;        /* DIAG clock register (read only) */
};

struct audio_descriptor {
    caddr_t audio_regs;
    struct audio_descriptor *next;
    int     audio2flag;
    int     min_bufsize;
    int     max_rcv_bufsize;
    int     max_snd_bufsize;
    int     rcv_bufsize;
    int     rcv_bufcount;
    unsigned char *rcv_buffer;
    unsigned char *rcv_buftail;
    unsigned char *rcv_bufhead;
    int     snd_bufsize;
    int     snd_bufcount;
    unsigned char *snd_buffer;
    unsigned char *snd_buftail;
    unsigned char *snd_bufhead;
    int     beep_bufsize;
    int     beep_bufdatalen;
    int     beep_bufcount;
    int     beep_frequency;          /* Audio II only */
    int     beep_accum_step;         /* Audio II only */
    short   *beep_wave_table;        /* Audio II only */
    unsigned char *beep_buffer;
    unsigned char *beep_bufptr;
    unsigned char *rcv_extra_buffer; /* Audio II only */
    unsigned char *rcv_extra_ptr;    /* Audio II only */
    unsigned char *rcv_meter_ptr;    /* Audio II only */
    unsigned char *snd_extra_buffer; /* Audio II only */
    unsigned char *snd_extra_ptr;    /* Audio II only */
    unsigned char *snd_meter_ptr;    /* Audio II only */
    int     rcurpage_cnt;            /* Audio II only */
    int     rnxtpage_cnt;            /* Audio II only */
    int     scurpage_cnt;            /* Audio II only */
    int     snxtpage_cnt;            /* Audio II only */
    int     snd_minus_rcv_cnt;       /* Audio II only */
    int     rcv_sel_threshold;
    int     snd_sel_threshold;
    int select_code;
    int flags;
    struct channel_gain cur_gain[2];
    int audio_output;
    int audio_input;
    int audio_format;
    int audio_sample_rate;
    int rcv_overflow_cnt;
    int snd_underflow_cnt;
    int open_count;
    struct proc *audio_rsel;
    struct proc *audio_wsel;
    int beep_save_16bit_flag;               /* Audio I only */
    unsigned long   receive_nsamples;
    unsigned long   receive_sample_sum[2];
    unsigned long   receive_sample_peak[2];
    unsigned long   receive_sample_save;    /* Audio I only */
    unsigned long   transmit_nsamples;
    unsigned long   transmit_sample_sum[2];
    unsigned long   transmit_sample_peak[2];
    unsigned long   transmit_sample_save;   /* Audio I only */
    unsigned short  receive_meter_state;
    unsigned short  transmit_meter_state;
    struct psb2160conf beep_save_config;    /* Audio I only */
    struct psb2160conf cur_config;          /* Audio I only */
    unsigned long   beep_save_cntl;         /* Audio II only */
    unsigned long   beep_save_gain;         /* Audio II only */
    unsigned long   cur_cntl;               /* Audio II only */
    unsigned long   cur_gainctl;            /* Audio II only */
    unsigned char   *test_buffer;           /* Audio II only */
};

#ifdef __hp9000s700
#define AUDIO_INT_LEVEL   5
#define AUDIO_1_CARD_ID           0x7a
#define AUDIO_1_NOBEEPER_CARD_ID  0x7e
#define AUDIO_2_CARD_ID           0x7f
#define AUDIO_2_NOBEEPER_CARD_ID  0x7b
#define AUDIO_2_HPA               0xf1000000

#define IRQ_SGC_SLOT1     20
#define IRQ_SGC_SLOT2     19
#define IRQ_AUDIO_CORE    18
#else
#define AUDIO_INT_LEVEL   6
#define AUDIO_1_CARD_ID   0x13
#define AUDIO_2_CARD_ID   0x14
#define AUDIO_1_NOBEEPER_CARD_ID  0x13
#define AUDIO_2_NOBEEPER_CARD_ID  0x14
#endif


/* Defines for flags */

#define AUD_F_OPEN        0x00000001
#define AUD_F_16BIT       0x00000002
#define AUD_F_RCVPAUSE    0x00000004
#define AUD_F_SNDPAUSE    0x00000008
#define AUD_F_RSLEEP      0x00000010
#define AUD_F_WSLEEP      0x00000020
#define AUD_F_RCOLL       0x00000040
#define AUD_F_WCOLL       0x00000080
#define AUD_F_BEEP        0x00000100
#define AUD_F_SOUNDBEEP   0x00000200
#define AUD_F_RWOPEN      0x00000400
#define AUD_F_ODD         0x00000800
#define AUD_F_RCVON       0x00001000
#define AUD_F_SNDON       0x00002000  /* Audio II only */
#define AUD_F_SNDLASTPG   0x00004000  /* Audio II only */
#define AUD_F_RCVLASTPG   0x00008000  /* Audio II only */
#define AUD_F_SNDFLUSH    0x00010000  /* Audio II only */
#define AUD_F_STEREO      0x00020000  /* Audio II only */
#define AUD_F_TESTMODE    0x00040000  /* Audio II only */
#define AUD_F_BUFVIACTL   0x00080000
#define AUD_F_NOBEEPER    0x00100000
#define AUD_F_BEEPCONF    0x00200000
#define AUD_F_RCVDRAIN    0x00400000  /* Audio II only */
#define AUD_F_MTRNOSUM    0x00800000  /* Audio II only */

/* Defines for interrupt register */

#define INFIFO_ENA        0x01
#define CTLFIFO_ENA       0x04
#define DATA16_MODE       0x08
#define INT_REQ           0x40
#define INT_ENA           0x80

/* Defines for fifointctl register */

#define IN_HALF_INT_REQ   0x01
#define OUT_HALF_INT_REQ  0x02
#define CTL_EMPTY_INT_REQ 0x04
#define OUT_EMPTY_INT_REQ 0x08
#define IN_HALF_INT_ENA   0x10
#define OUT_HALF_INT_ENA  0x20
#define CTL_EMPTY_INT_ENA 0x40
#define OUT_EMPTY_INT_ENA 0x80

/* Defines for fifostatus register */

#define IN_EMPTY          0x01
#define IN_FULL           0x02
#define OUT_EMPTY         0x04
#define OUT_FULL          0x08
#define CTL_EMPTY         0x10
#define CTL_FULL          0x20

/* Defines for Audio commands */

#define AC_SOP_0          0x20
#define AC_COP_1          0x21
#define AC_COP_2          0x22
#define AC_COP_3          0x23
#define AC_SOP_4          0x24
#define AC_SOP_5          0x25
#define AC_SOP_6          0x26
#define AC_SOP_7          0x27
#define AC_COP_8          0x28
#define AC_COP_9          0x29
#define AC_COP_A          0x2a
#define AC_COP_B          0x2b
#define AC_COP_C          0x2c
#define AC_COP_D          0x2d
#define AC_COP_E          0x2e
#define AC_NOP            0x2f

/* Defines for audio_beeper() beep type */

#define BEEPTYPE_200      0 /* Series 200 compatible         */
#define BEEPTYPE_300      1 /* Series 300 compatible         */
#define BEEPTYPE_GENERAL  2 /* General, ala struct beep_info */

#endif /* _KERNEL */
#endif /* _SYS_AUDIO_INCLUDED */
