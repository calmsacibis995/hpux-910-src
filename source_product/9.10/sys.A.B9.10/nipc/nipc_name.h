/*
 * @(#)nipc_name.h: $Revision: 1.3.83.4 $ $Date: 93/09/17 19:10:51 $
 * $Locker:  $
 */

/*
 *  Header file for node names and socket names. 
 */
#ifndef NIPC_NAME.H
#define NIPC_NAME.H


struct nm_lookuphdr {
	short	hdr_msglen;
	char	hdr_pid;
	char	hdr_msgtype;
	int	hdr_seqnum;
	int	hdr_capmask;
	short	hdr_error;
	char	hdr_version;
	char	hdr_unused;
};
	
struct nm_lookupreq {
	struct nm_lookuphdr	req_hdr;
	short			req_nameptr;
	short			req_endptr;
};

struct nm_lookupreply {
	struct nm_lookuphdr	reply_hdr;
	short			reply_count;	/* number of reports in reply */
	short			reply_sockkind;
};
	
#define NM_SUCCESS_RESULT	0	/* reply error code - successful      */
#define NM_VERSION_RESULT	1	/* reply error code - bad version     */
#define NM_NOTFOUND_RESULT	2	/* reply error code - name not found  */

#define NM_REQUEST		1	/* lookup message type - request      */
#define NM_REPLY		2	/* lookup message type - reply	      */
#define NM_CAPABILITY		0	/* lookup header capability mask      */
#define NM_PID			10	/* lookup header protocol id	      */
#define NM_VERSION		0	/* lookup header version number       */

#define NM_MAX_REPLY_LEN	520	/* maximum request/reply message len  */
#define NM_MAX_REQ_LEN		36	/* maximum request message length     */
#define NM_HDR_LEN		16	/* fixed portion of request/reply hdr */
#define NM_REQ_LEN		20	/* valid request header length	      */
#define NM_REPLY_LEN		20	/* successful reply header length     */

#define NM_SERVER		1541	/* well known port addr for sockregd  */
#define NM_SOCKLEN		8	/* socket name length of random name  */
#define NM_MAXPARTS		3	/* max number of parts in a node name */

#define BYTE_ALIGN	0x1
#endif /* NIPC_NAME.H*/
