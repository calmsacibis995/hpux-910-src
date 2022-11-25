/* RCS ID: @(#) $Header: message.h,v 66.10 91/01/01 15:26:04 rpt Exp $ */
/* Copyright (c) HP, 1989 */

#ifndef MESSAGEincluded
#define MESSAGEincluded

#define MESSAGEsoftKeysFile            message[1]
#define MESSAGEdefaultRCFile           message[2]
#define MESSAGEhelpFile                message[3]
#define MESSAGEintrinsicsFile          message[4]
#define MESSAGEuserSoftKeysFile        message[5]
#define MESSAGEuserRCFile              message[6]
#define MESSAGE7                       message[7]
#define MESSAGE8                       message[8]
#define MESSAGE9                       message[9]

#define MESSAGEterminfoFailed          message[10]
#define MESSAGEexecingKsh              message[11]
#define MESSAGEcontinuing              message[12]
#define MESSAGEcannotFindSoftKeyInFile message[13]
#define MESSAGEcannotFindSoftKey       message[14]
#define MESSAGEconflictWritingConfig   message[15]
#define MESSAGEcannotWriteConfig       message[16]
#define MESSAGEcannotUnlink            message[17]
#define MESSAGEautoConfigError         message[18]
#define MESSAGEcannotOpenSoftKeyFile   message[19]
#define MESSAGEunknownUser             message[20]
#define MESSAGEerrorSoftKeyFile        message[21]
#define MESSAGEsoftKeyExpected         message[22]
#define MESSAGEattributeExpected       message[23]
#define MESSAGEsemiColonExpected       message[24]
#define MESSAGEcurlyExpected           message[25]
#define MESSAGEnonZeroCodeExpected     message[26]
#define MESSAGEtooManyErrors           message[27]
#define MESSAGEparseError              message[28]
#define MESSAGEhelpCorrupt             message[29]
#define MESSAGEcannotReadConfig        message[30]
#define MESSAGE31                      message[31]
#define MESSAGE32                      message[32]
#define MESSAGE33                      message[33]
#define MESSAGE34                      message[34]
#define MESSAGE35                      message[35]
#define MESSAGE36                      message[36]
#define MESSAGE37                      message[37]
#define MESSAGE38                      message[38]
#define MESSAGE39                      message[39]

#define MESSAGEhelpLabel               message[40]
#define MESSAGEhelpAccelerator         message[41]
#define MESSAGEetcLabel                message[42]
#define MESSAGEetcAccelerator          message[43]
#define MESSAGEetcOfLabel              message[44]

#define MESSAGEnewMail                 message[45]
#define MESSAGEoldMail                 message[46]
#define MESSAGEnoMail                  message[47]
#define MESSAGE48                      message[48]
#define MESSAGE49                      message[49]

#define MESSAGEhelpHint                message[50]
#define MESSAGEnoHelpHint              message[51]
#define MESSAGEnoHintHint              message[52]
#define MESSAGEnoHelpHelp              message[53]
#define MESSAGEdisabledHint            message[54]
#define MESSAGEhelpSoftKey             message[55]
#define MESSAGEgeneralHelpHint         message[56]
#define MESSAGEhelpTopicsLabel         message[57]
#define MESSAGEhelpQuit                message[58]
#define MESSAGE59                      message[59]

#define MESSAGEkeyshellIntrinsic       message[60]
#define MESSAGEkeyshellIntrinsicAlias  message[61]
#define MESSAGEpressSpace              message[62]
#define MESSAGEtsmWindow               message[63]
#define MESSAGEincomplete              message[64]
#define MESSAGEpreamble                message[65]
#define MESSAGEselectAnyOptions        message[66]
#define MESSAGEselectAnOption          message[67]
#define MESSAGEnotAllowedHere          message[68]
#define MESSAGE69                      message[69]

extern char *message[];

extern void MessageInitialize();

#endif

