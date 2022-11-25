#ifdef __hp9000s800
/* HPUX_ID: @(#) $Revision: 66.1 $  */
#include "sio_cs80lib.h"

/*
 * buf[119] = mode  DESTRUCTIVE/NONDESTRUCTIVE
 * buf[120] = transaction type
 * buf[121] = command message length
 * buf[122] - buf[123] = timeout value
 * buf[124] - buf[127] = execution message length
 * 
 * fd = file descriptor of device file
 * comp = complementary buffer
 * buf = character buffer that contains command information
 * len = execution message length
 * to = timeout value in seconds
 */

xcbld(comp,bufr,length)
comp_type	*comp;
char		*bufr;
int		*length;
{
    int		c, b;

    b = 0;

    if (comp->unit != -1) {
	bufr[b] = SETUNIT | (comp->unit & 0x0f);
	b++;
    }

    if (comp->volume != -1) {
	bufr[b] = SETVOLUME | (comp->volume & 0x07);
	b++;
    }

    if (comp->address_mode != -1) {
	bufr[b] = SETADDRESS | (comp->address_mode &0x01);
	b++;
	for (c=0; c<6; c++) {
	    bufr[b] = comp->address[c];
	    b++;
        }
    }

    if (comp->disp_flag != -1) {
	bufr[b] = SETDISPLACEMENT;
	b++;
	for (c=0; c<6; c++) {
	    bufr[b] = comp->disp[c];
	    b++;
	}
    }

    if (comp->len_flag != -1) {
	bufr[b] = SETLENGTH;
	b++;
	for (c=0; c<4; c++) {
	    bufr[b] = comp->len[c];
	    b++;
	}
    }

/*
 * Bursting not supported by CS80 driver.
 *
 *  if (comp->burst != -1) {
 *	bufr[b] = SETBURST;
 *	if (comp->burst &0x80) bufr[b] |= 0x01;
 *	b++;
 *      bufr[b] = comp->burst & 0x7f;
 *	b++;
 *  }
 */

    if (comp->rps_time1 != -1) {
	bufr[b] = SETRPS;
	b++;
	bufr[b] = comp->rps_time1;
	b++;
	bufr[b] = comp->rps_time2;
	b++;
    }

    if (comp->retry_time != -1) {
	bufr[b] = SETRETRYTIME;
	b++;
	bufr[b] = (comp->retry_time >> 8) & 0xff;
	b++;
	bufr[b] = comp->retry_time & 0xff;
	b++;
    }

    if (comp->status_flag != -1) {
	bufr[b] = SETMASK;
	b++;
	for (c=0; c<8; c++) {
	    bufr[b] = comp->status_mask[c];
	    b++;
	}
    }

    if (comp->release != -1) {
	bufr[b] = SETRELEASE;
	b++;
	bufr[b] = (comp->release & 0x03) << 6;
	b++;
    }

    if (comp->rtn_addr_mode != -1) {
	bufr[b] = SETRTNADDRMODE;
	b++;
	bufr[b] = comp->rtn_addr_mode & 0x07;
	b++;
    }

    if (comp->options != -1) {
	bufr[b] = SETOPTION;
	b++;
	bufr[b] = comp->options;
	b++;
    }

    *length = b;

    return;
}

xcicl(fd,comp,buf,len,to)	/* Channel Independent Clear */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		i, ii, clen;

    xcbld(comp,buf,&clen);
    buf[clen] = CHANNELCLEAR;	/* must be a set unit in comp */
    unpk32(to,buf,120);
    buf[121] = ++clen;
    buf[120] = TRANS_WR;
    buf[119] = DESTRUCTIVE;
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xcold(fd,comp,buf,len,to)	/* Cold Load Read */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		ii, i, clen;

    xcbld(comp,buf,&clen);
    buf[clen] = COLDLOAD;
    unpk32(to,buf,120);
    buf[121] = ++clen;
    buf[120] = CS80_WRR;
    buf[119] = DESTRUCTIVE;
    if (len == 0) 		/* may also want to check set length */
	buf[120] = CS80_WR;
    else {
        buf[120] = CS80_WRR;
        unpk32(len,buf,124);
    }
    for (i=64;i<= 110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xcomp(fd,comp,buf,len,to)	/* Complementary Commands */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
	int	i, ii, clen;

	xcbld(comp,buf,&clen);
	if (clen == 0) {	/* send NOP if no comp. commands */
	    buf[clen] = NOOP;
	    clen++;
	}
        unpk32(to,buf,120);
	buf[121] = clen;
	buf[120] = CS80_WR;
	if ((comp->volume != -1) || (comp->disp_flag != -1) || 
	    (comp->rps_time1 != -1) || (comp->retry_time != -1) || 
	    (comp->status_flag != -1) || (comp->release != -1) || 
	    (comp->rtn_addr_mode != -1) || (comp->options != -1)) {
            buf[119] = DESTRUCTIVE;
	}
	else 
	    buf[119] = NON_DESTRUCTIVE; /* Set unit, set len, set addr are 
					  non-destructive commands */
        for (i=64;i<= 110;i++) buf[i] = 0;
	i = read(fd,buf,len+128);
	return(i);
}

xcopy(fd,comp,buf,len,to)	/* Copy Data */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		i, ii, clen;

    /*
     * buf[64] = source unit (0-7)
     * buf[65] = source volume (0-7)
     * buf[66] = address mode (0=single vector, 1=3 vector)
     * buf[68] - buf[73] = address (source)
     * buf[74] = destination unit (0-7)
     * buf[75] = destination volume (0-7)
     * buf[76] = address mode (0=single vector, 1=3 vector)
     * buf[78] - buf[81] = address (destination)
     */
    xcbld(comp,buf,&clen);
    buf[clen] = COPYDATA;
    clen++;
    buf[clen] = (buf[64] & 0x07) | ((buf[65] & 0x07) << 4);
    clen++;
    buf[clen] = SETADDRESS | (buf[66] & 0x01);
    clen++;
    for (i=0;i<6;i++) buf[clen+i] = buf[68+i];
    clen += 6;
    buf[clen] = (buf[74] & 0x07) | ((buf[75] & 0x07) << 4);
    clen++;
    buf[clen] = SETADDRESS | (buf[76] & 0x01);
    clen++;
    for (i=0;i<6;i++) buf[clen+i] = buf[68+i];
    unpk32(to,buf,120);
    buf[121] = clen + 6;
    buf[120] = CS80_WR;
    buf[119] = DESTRUCTIVE;
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xcncl(fd,comp,buf,len,to)	/* Cancel */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		i, ii, clen;

    xcbld(comp,buf,&clen);
    buf[clen] = CANCEL;
    unpk32(to,buf,120);
    buf[121] = ++clen;
    buf[120] = TRANS_WR;
    buf[119] = DESTRUCTIVE;
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xdesc(fd,comp,buf,len,to)	/* Describe Unit */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		ii, i, clen;

    xcbld(comp,buf,&clen);
    buf[clen] = DESCRIBE;
    unpk32(to,buf,120);
    buf[121] = ++clen;
    buf[120] = CS80_WRR;
    buf[119] = NON_DESTRUCTIVE;
    unpk32(len,buf,124);
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xexdesc(fd,comp,buf,len,to)	/* Extended Describe Unit */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		ii, i, clen;

    xcbld(comp,buf,&clen);
    buf[clen] = EXT_DESCRIBE;
    unpk32(to,buf,120);
    buf[121] = ++clen;
    buf[120] = CS80_WRR;
    buf[119] = NON_DESTRUCTIVE;
    unpk32(len,buf,124);
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xdiag(fd,comp,buf,len,to)	/* Execute Diagnostic */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		i, ii, clen;

    /* buf[64] - buf[65] = loop count
     * buf[66] = diagnostic section number
     */

    xcbld(comp,buf,&clen);
    buf[clen] = INITDIAGNOSTIC;
    clen++;
    for (i=0;i<3;i++) buf[clen+i] = buf[64+i];
    unpk32(to,buf,120);
    buf[121] = clen + 3;
    buf[120] = CS80_WR;
    if (buf[66] == 0)
        buf[119] = NON_DESTRUCTIVE;
    else
        buf[119] = DESTRUCTIVE;
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xfmrk(fd,comp,buf,len,to)	/* Write File Mark */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		i, ii, clen;

    xcbld(comp,buf,&clen);
    buf[clen] = WRITEMARK;
    unpk32(to,buf,120);
    buf[121] = ++clen;
    buf[120] = CS80_WR;
    buf[119] = DESTRUCTIVE;
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xdvlk(fd,comp,buf,len,to)	/* Device Lock */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		i, ii, clen;

    xcbld(comp,buf,&clen);
    buf[clen] = DEVICE_LOCK;
    unpk32(to,buf,120);
    buf[121] = ++clen;
    buf[120] = CS80_WR;
    buf[119] = DESTRUCTIVE;
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xdvulk(fd,comp,buf,len,to)	/* Device Unlock */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		i, ii, clen;

    xcbld(comp,buf,&clen);
    buf[clen] = DEVICE_UNLOCK;
    unpk32(to,buf,120);
    buf[121] = ++clen;
    buf[120] = CS80_WR;
    buf[119] = NON_DESTRUCTIVE;
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xnrh(fd,comp,buf,len,to)	/* Non Responding Host Timeout*/
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		i, ii, clen;

    /*
     * buf[64] - buf[65] = timeout value
     */

    xcbld(comp,buf,&clen);
    buf[clen] = NRH_TO;
    clen++;
    for (i=0;i<2;i++) buf[clen+i] = buf[64+i];
    unpk32(to,buf,120);
    buf[121] = clen + 2;
    buf[120] = CS80_WR;
    buf[119] = NON_DESTRUCTIVE;
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xinmd(fd,comp,buf,len,to)	/* Initialize Media */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		i, ii, clen;

    /* buf[64] = options
     * buf[65] = interleave factor
     */

    xcbld(comp,buf,&clen);
    buf[clen] = INITMEDIA;
    clen++;
    buf[clen] = buf[64];
    clen++;
    buf[clen] = buf[65];
    unpk32(to,buf,120);
    buf[121] = ++clen;
    buf[120] = CS80_WR;
    buf[119] = DESTRUCTIVE;
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xlcrd(fd,comp,buf,len,to)	/* Locate and Read */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		ii, i, clen;

    xcbld(comp,buf,&clen);
    buf[clen] = LOCATENREAD;
    unpk32(to,buf,120);
    buf[121] = ++clen;
    if (len == 0)		/* should also ckeck set length */
	buf[120] = CS80_WR;
    else {
        buf[120] = CS80_WRR;
        unpk32(len,buf,124);
    }
    buf[119] = NON_DESTRUCTIVE;
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xlcvf(fd,comp,buf,len,to)	/* Locate and Verify */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		i, ii, clen;

    xcbld(comp,buf,&clen);
    buf[clen] = LOCATENVERIFY;
    unpk32(to,buf,120);
    buf[121] = ++clen;
    buf[120] = CS80_WR;
    buf[119] = NON_DESTRUCTIVE;
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xlcwr(fd,comp,buf,len,to)	/* Locate and Write */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		i, ii, clen;

    xcbld(comp,buf,&clen);
    buf[clen] = LOCATENWRITE;
    unpk32(to,buf,120);
    buf[121] = ++clen;
    buf[120] = CS80_WWR;
    buf[119] = DESTRUCTIVE;
    unpk32(len,buf,124);
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xreld(fd,comp,buf,len,to)	/* Release Denied */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		i, ii, clen;

    xcbld(comp,buf,&clen);
    buf[clen] = RELEASEDENIED;
    unpk32(to,buf,120);
    buf[121] = ++clen;
    buf[120] = CS80_WR;
    buf[119] = DESTRUCTIVE;
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xrels(fd,comp,buf,len,to)	/* Release */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		i, ii, clen;

    xcbld(comp,buf,&clen);
    buf[clen] = RELEASE;
    unpk32(to,buf,120);
    buf[121] = ++clen;
    buf[120] = CS80_WR;
    buf[119] = DESTRUCTIVE;
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xrlpb(fd,comp,buf,len,to)	/* Read Loopback */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		ii, i, clen;

    /* buf[64] - buf[67] = loopback data length */
    /* no complementary commands allowed */
    clen = 0;
    buf[clen] = READLOOPBACK;
    clen++;
    for (i=0;i<4;i++) buf[clen+i] = buf[64+i];
    unpk32(to,buf,120);
    buf[121] = clen + 4;
    buf[120] = TRANS_WRR;
    buf[119] = NON_DESTRUCTIVE;
    unpk32(len,buf,124);
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xrqst(fd,comp,buf,len,to)	/* Request Status */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		ii, i, clen;

    xcbld(comp,buf,&clen);
    buf[clen] = REQSTATUS;
    unpk32(to,buf,120);
    buf[121] = ++clen;
    buf[120] = CS80_WRR;
    buf[119] = NON_DESTRUCTIVE;
    unpk32(len,buf,124);
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xspre(fd,comp,buf,len,to)	/* Spare Block */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		i, ii, clen;

    /* buf[64] = spare mode */
    xcbld(comp,buf,&clen);
    buf[clen] = SPAREBLOCK;
    clen++;
    buf[clen] = buf[64];
    unpk32(to,buf,120);
    buf[121] = ++clen;
    buf[120] = CS80_WR;
    buf[119] = DESTRUCTIVE;
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xunld(fd,comp,buf,len,to)	/* Unload */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		i, ii, clen;

    xcbld(comp,buf,&clen);
    buf[clen] = UNLOAD;
    unpk32(to,buf,120);
    buf[121] = ++clen;
    buf[120] = CS80_WR;
    buf[119] = DESTRUCTIVE;
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xutil(fd,comp,buf,len,to)	/* Execute Utiltiy */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		i, ii, clen;

    /* buf[64] - buf[67] = execution message length
     * buf[68] = excution msg qualifier (0=none, 1=to device, 2=from device)
     * buf[69] = utility number
     * buf[70] - buf[77] = parameter bytes
     * buf[78] = number of parameter bytes
     */

    xcbld(comp,buf,&clen);
    buf[clen] = INITUTILITY | (buf[68] & 03);
    clen++;
    buf[clen] = buf[69];
    clen++;
    for (i=0;i<buf[78];i++) buf[clen+i] = buf[70+i];
    unpk32(to,buf,120);
    buf[121] = clen + buf[78];
    buf[120] = CS80_WR;
    /* Flag the destructive utilities */
    switch (buf[69]) {
    case 200:  /* Pattern ERT */
    case 202:  /* Short ERT */
    case 203:  /* Random ERT */
    case 205:  /* Clear Logs */
    case 206:  /* Preset */
	buf[119] = DESTRUCTIVE;
  	break;
    default:
    	buf[119] = NON_DESTRUCTIVE;
	break;
    }
    switch (buf[68]) {
    case 0:
	buf[120] = CS80_WR;
	break;

    case 1:
	buf[120] = CS80_WWR;
	break;

    case 2:
	buf[120] = CS80_WRR;
	break;
    }
    for (i=0;i<4;i++) buf[124+i] = buf[64+i];
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xwlpb(fd,comp,buf,len,to)	/* Write Loopback */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		ii, i, clen;

    /* buf[64] - buf[67] = loopback data length */
    /* no complementary commands allowed */
    clen = 0;
    buf[clen] = WRITELOOPBACK;
    clen++;
    for (i=0;i<4;i++) buf[clen+i] = buf[64+i];
    unpk32(to,buf,120);
    buf[121] = clen + 4;
    buf[120] = TRANS_WWR;
    buf[119] = NON_DESTRUCTIVE;
    unpk32(len,buf,124);
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xrhr(fd,comp,buf,len,to)	/* Remote Host Reset*/
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		ii, i, clen;

    /* buf[64] = host port number */
    /* no complementary commands allowed */
    clen = 0;
    buf[clen] = RHRESET;
    clen++;
    buf[clen] = buf[64];
    unpk32(to,buf,120);
    buf[121] = ++clen;
    buf[120] = TRANS_WR;
    buf[119] = DESTRUCTIVE;
    unpk32(len,buf,124);
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xtstat(fd,comp,buf,len,to)	/* Transparent Status */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		ii, i, clen;

    /* no complementary commands allowed */
    clen = 0;
    buf[clen] = TRANS_STAT;
    clen++;
    unpk32(to,buf,120);
    buf[121] = clen;
    buf[120] = TRANS_WR;
    buf[119] = NON_DESTRUCTIVE;
    unpk32(len,buf,124);
    for (i=64;i<=110;i++) buf[i] = 0;
    i = read(fd,buf,len+128);
    return(i);
}

xalpb(fd,comp,buf,len,to)	/* Alink/Amux Loopback */
int		fd, len;
char		*buf;
comp_type	*comp;
int             to;
{
    int		ii, i, clen;

    /* buf[64] - type of loopback
     *	INT_LOOPBACK	1
     *	WKVC_LOOPBACK	2
     *	DEV_LOOPBACK	3
     *	EXTRN_LOOPBACK	4
     *	CIO_LOOPBACK	5
     */
    /* no complementary commands allowed */
    unpk32(to,buf,120);
    buf[121] = 0; /* no command message */
    buf[119] = NON_DESTRUCTIVE;
    unpk32(len,buf,124);
    switch (buf[64]) {
	case 1:	/* internal loopback */
	    buf[120] = INT_LB;
	    buf[119] = DESTRUCTIVE;
	    break;

	case 2:	/* Well known virtual circuit loopback */
	    buf[120] = WKVC_LB;
	    break;

	case 3:	/* Device loopback */
	    buf[120] = DEV_E_LB;
	    break;

	case 4:	/* External loopback */
	    buf[120] = DEV_E_LB;
	    buf[119] = DESTRUCTIVE;
	    break;

	case 5:	/* CIO loopback */
	    buf[120] = CIO_LB;
	    break;

    } /* end of switch */
    for (i=64;i<=110;i++) buf[i] = 0;
    unpk32(len,buf,124);
    i = read(fd,buf,len+128);
    return(i);
}
#endif /* __hp9000s800 */
