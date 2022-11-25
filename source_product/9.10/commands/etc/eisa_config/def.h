/*****************************************************************************
 (C) Copyright Hewlett-Packard Co. 1991. All rights
 reserved.  Copying or other reproduction of this program except for archival
 purposes is prohibited without prior written consent of Hewlett-Packard.

			  RESTRICTED RIGHTS LEGEND

Use, duplication, or disclosure by Government is subject to restrictions
set forth in paragraph (b) (3) (B) of the Rights in Technical Data and
Computer Software clause in DAR 7-104.9(a).

HEWLETT-PACKARD COMPANY
Fort Collins Engineering Operation, Ft. Collins, CO 80525

******************************************************************************/
/******************************************************************************
*   (C) Copyright COMPAQ Computer Corporation 1985, 1989
*******************************************************************************
*
*                         inc/def.h
*
*      These are some basic defines used by the cfg file parser and friends.
*
*******************************************************************************/


#define BIT0		1
#define BIT15		32768
#define BIT31		0x80000000

#define ISA8_SLOT	0
#define ISA16_SLOT	1
#define ISA8OR16_SLOT	2
#define ISA32_SLOT	3
#define EISA_SLOT	4
#define EMB_SLOT	5
#define VIR_SLOT	6
#define OTH_SLOT	7

#define DIP_TYPE	0
#define ROTARY_TYPE	1
#define SLIDE_TYPE	2

#define PAIRED_TYPE	0
#define INLINE_TYPE	1
#define TRIPOLE_TYPE	2

#define SWITCH_BLOCK	0
#define JUMPER_BLOCK	1
#define IOPORT_BLOCK	2

#define LINKED_GROUP	0
#define COMBINED_GROUP	1
#define FREE_GROUP	2

#define DMA_TYPE	0
#define IRQ_TYPE	1
#define PORT_TYPE	2
#define MEMORY_TYPE	3
#define ADDRESS_TYPE	4

#define DEF_TIMING	0
#define TYPEA_TIMING	1
#define TYPEB_TIMING	2
#define TYPEC_TIMING	3

#define IRQ_EDGE	0
#define IRQ_LEVEL	1

#define SYS_MEM 	0
#define EXP_MEM 	1
#define OTHER_MEM	2
#define VIR_MEM 	3

#define SWITCH_INIT	0
#define JUMPER_INIT	1
#define IOPORT_INIT	2
#define SOFT_INIT	3

#define BOARD_ID_REQ	0
#define BOARD_NAME_REQ	1
#define BOARD_MAN_REQ	2
#define BOARD_CAT_REQ	3
#define CTRL_NAME_REQ	4
#define CTRL_TYPE_REQ	5
#define FUNCTION_REQ	6
#define ADDRESS_REQ	7
#define CHOICE_REQ	8

#define NAME_MAX_LEN		90
#define MFR_MAX_LEN		30
#define GROUP_MAX_LEN		100
#define FUNC_MAX_LEN		100
#define CHOICE_MAX_LEN		90
#define COMM_MAX_LEN		600
#define HELP_MAX_LEN		600
#define CTRL_NAME_MAX_LEN	20
#define LABEL_MAX_LEN		10
#define SOFT_MAX_LEN		600
#define SOFT_PARM_MAX_LEN	20
#define TYPE_MAX_LEN		80
#define CONNECT_MAX_LEN 	600
#define SHARE_MAX_LEN		10
#define SLOT_TAG_MAX_LEN	10
#define SLOT_LABEL_MAX_LEN	8

#define BOARD_MAX_LEN		999
#define MAXIMUM_AMP		99999
#define BUSMASTER_MAX		999
#define SYS_NVMEM_MAX		65536
#define SYS_SLOT_LEN_MAX	999
#define CTRL_NDX_MAX		255

#define MAX_DMA 		7
#define MAX_IRQ 		15
#define MAX_PORT		0xFFFF
#define MAX_MEMORY		0x4000000

#define EMB_SLOT_MAX		15
#define FREEFORM_MAX		203

#define DEFAULT_BOARD_LENGTH	330
#define DEFAULT_SLOT_LENGTH	341
#define DEFAULT_SIZING_GAP	0

#define MEMORY_GRANULARITY	0x400
#define ADDRESS_GRANULARITY	0x100

#define CONVERT_EMB_CHAR	1
#define REMOVE_EMB_CHAR		2

#define LONG_STRING		1
#define SHORT_STRING		2
