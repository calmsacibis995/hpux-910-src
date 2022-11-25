/* @(#) $Revision: 62.1 $ */   

#include "arab_def.h"

unsigned char    arabfnts[] = 

{

/* Normal Diacritic font array -- DIAC_NORMAL            */

 '\141', /* tan_fatha                                    */
 '\142', /* tan_dham                                     */
 '\143', /* tan_kas                                      */
 '\147', /* fatha                                        */
 '\150', /* dhamma                                       */
 '\151', /* kasra                                        */
 '\153', /* shadda                                       */
 '\152', /* sukun                                        */
 '\101', /* char_bug                                     */
 '\101', /* char_bug                                     */
 '\101', /* char_bug                                     */
 '\144', /* shadda_fat                                   */
 '\145', /* shadda_dam                                   */
 '\146', /* shadda_kas                                   */

/* Medial Diacritic font array -- DIAC_MEDIAL            */

 '\373', /* fatha c                                      */
 '\374', /* dhamma_c                                     */
 '\375', /* kasra_c                                      */
 '\335', /* shadda_c                                     */
 '\376', /* sukun_c                                      */
 '\101', /* char_bug                                     */
 '\101', /* char_bug                                     */
 '\101', /* char_bug                                     */
 '\370', /* shadda_fat_c                                 */
 '\371', /* shadda_dham_c                                */
 '\372', /* shadda_kas_c                                 */

/* Baa Q Diacritic font array -- DIAC_BAA_Q              */

 '\243', /* tan_fat_bq                                   */
 '\244', /* tan_dham_bq                                  */
 '\245', /* tan_kas_bq                                   */
 '\251', /* tatha_bq                                     */
 '\252', /* dhamma_bq                                    */
 '\253', /* kas_bq                                       */
 '\255', /* shadda_bq                                    */
 '\254', /* sukun_bq                                     */
 '\101', /* char_bug                                     */
 '\101', /* char_bug                                     */
 '\101', /* char_bug                                     */
 '\246', /* shadda_fat_bq                                */
 '\247', /* shadda_dham_bq                               */
 '\250', /* shadda_kas_bq                                */

/* Seen Q Diacritic font array -- DIAC_SEEN_Q             */

 '\243', /* tan_fat_sq                                   */
 '\257', /* tan_dham_sq                                  */
 '\260', /* tan_kas_sq                                   */
 '\264', /* fatha_sq                                     */
 '\265', /* dhamma_sq                                    */
 '\266', /* kasra_sq                                     */
 '\270', /* shadda_sq                                    */
 '\267', /* sukun_sq                                     */
 '\101', /* char_bug                                     */
 '\101', /* char_bug                                     */
 '\101', /* char_bug                                     */
 '\261', /* shadda_fat_sq                                */
 '\262', /* shadda_dham_sq                               */
 '\263', /* shadda_kas_sq                                */
 
/* Miscellaneous char font array -- MISC_CHAR             */

 '\041', /* exclamation                                   */
 '\042', /* quote                                         */
 '\043', /* national money                                */
 '\044', /* international money                           */
 '\045', /* percent sign                                  */
 '\046', /* ampersand                                     */
 '\047', /* decimal comma                                 */
 '\051', /* open paren                                    */
 '\050', /* closing paren                                 */
 '\052', /* asterix                                       */
 '\053', /* plus sign                                     */
 '\054', /* arabic comma                                  */
 '\055', /* hypen                                         */
 '\056', /* period                                        */
 '\057', /* slant / solidus                               */
 '\060', /* india zero                                    */
 '\061', /* india one                                     */
 '\062', /* india two                                     */
 '\063', /* india three                                   */
 '\064', /* india four                                    */
 '\065', /* india five                                    */
 '\066', /* india six                                     */
 '\067', /* india seven                                   */
 '\070', /* india eight                                   */
 '\071', /* india nine                                    */
 '\072', /* colon                                         */
 '\073', /* arab semicolon                                */
 '\074', /* less than sign                                */
 '\075', /* equal sign                                    */
 '\076', /* greater than sign                             */
 '\077', /* question mark                                 */
 '\100', /* commerical at                                 */

/* Special char font array '\0 1'- SPEC_1                 */

 '\133', /* open bracket                                  */
 '\134', /* reverse slant                                 */
 '\135', /* close bracket                                 */
 '\136', /* up arrow                                      */
 '\137', /* underline                                     */

/* Special char font array n'\0 2'- SPEC_2                */

 '\173', /* open brace                                   */
 '\174', /* vertical line                                */
 '\175', /* close brace                                  */
 '\176', /* over line                                    */

/* Arabic mapping array: codar-u to font codes -- ARAB_MAP */ 
/*    font                      codar u                    */
/*    code   name               code    context            */

 '\140', /* hamza               193     init             */
 '\140', /*                             med              */
 '\140', /*                             final            */
 '\140', /*                             iso              */
 '\316', /* alif madda          194     init             */
 '\316', /*                             med              */
 '\316', /*                             final            */
 '\316', /*                             iso              */
 '\315', /* alif hamza          195     init             */
 '\315', /*                             med              */
 '\315', /*                             final            */
 '\315', /*                             iso              */
 '\307', /* waw hamza           196     init             */
 '\307', /*                             med              */
 '\307', /*                             final            */
 '\307', /*                             iso              */
 '\317', /* hamza under alif    197     init             */
 '\317', /*                             med              */
 '\317', /*                             final            */
 '\317', /*                             iso              */
 '\330', /* hamza on cursy      198     init             */
 '\330', /*                             med              */
 '\166', /*                             final            */
 '\305', /*                             iso              */
 '\320', /* alif                199     init             */
 '\164', /*                             med              */
 '\164', /*                             final            */
 '\320', /*                             iso              */
 '\336', /* baa                 200     init             */
 '\336', /*                             med              */
 '\336', /*                             final            */
 '\336', /*                             iso              */
 '\162', /* taa marbuta         201     init             */
 '\163', /*                             med              */
 '\163', /*                             final            */
 '\162', /*                             iso              */
 '\337', /* taa                 202     init             */
 '\337', /*                             med              */
 '\337', /*                             final            */
 '\337', /*                             iso              */
 '\340', /* thaa                203     init             */
 '\340', /*                             med              */
 '\340', /*                             final            */
 '\340', /*                             iso              */
 '\345', /* jeem                204     init             */
 '\353', /*                             med              */
 '\274', /*                             final            */
 '\157', /*                             iso              */
 '\346', /* ha                  205     init             */
 '\354', /*                             med              */
 '\275', /*                             final            */
 '\160', /*                             iso              */
 '\347', /* khaa                206     init             */
 '\355', /*                             med              */
 '\276', /*                             final            */
 '\161', /*                             iso              */
 '\310', /* daal                207     init             */
 '\310', /*                             med              */
 '\310', /*                             final            */
 '\310', /*                             iso              */
 '\311', /* thaal               208     init             */
 '\311', /*                             med              */
 '\311', /*                             final            */
 '\311', /*                             iso              */
 '\312', /* raa                 209     init             */
 '\312', /*                             med              */
 '\312', /*                             final            */
 '\312', /*                             iso              */
 '\313', /* zaa                 210     init             */
 '\313', /*                             med              */
 '\313', /*                             final            */
 '\313', /*                             iso              */
 '\364', /* seen                211     init             */
 '\364', /*                             med              */
 '\331', /*                             final            */
 '\331', /*                             iso              */
 '\365', /* sheen               212     init             */
 '\365', /*                             med              */
 '\332', /*                             final            */
 '\332', /*                             iso              */
 '\366', /* saad                213     init             */
 '\366', /*                             med              */
 '\333', /*                             final            */
 '\333', /*                             iso              */
 '\367', /* dhaad               214     init             */
 '\367', /*                             med              */
 '\334', /*                             final            */
 '\334', /*                             iso              */
 '\325', /* ta                  215     init             */
 '\325', /*                             med              */
 '\325', /*                             final            */
 '\325', /*                             iso              */
 '\326', /* tha                 216     init             */
 '\326', /*                             med              */
 '\326', /*                             final            */
 '\326', /*                             iso              */
 '\342', /* ayn                 217     init             */
 '\350', /*                             med              */
 '\271', /*                             final            */
 '\154', /*                             iso              */
 '\343', /* ghayn               218     init             */
 '\351', /*                             med              */
 '\272', /*                             final            */
 '\155', /*                             iso              */
 '\101', /* char_bug            219     init             */
 '\101', /*                             med              */
 '\101', /*                             final            */
 '\101', /*                             iso              */
 '\101', /* char_bug            065     init             */
 '\101', /*                             med              */
 '\101', /*                             final            */
 '\101', /*                             iso              */
 '\101', /* char_bug            221     init             */
 '\101', /*                             med              */
 '\101', /*                             final            */
 '\101', /*                             iso              */
 '\101', /* char_bug            222     init             */
 '\101', /*                             med              */
 '\101', /*                             final            */
 '\101', /*                             iso              */
 '\101', /* char_bug            223     init             */
 '\101', /*                             med              */
 '\101', /*                             final            */
 '\101', /*                             iso              */
 '\327', /* tamdeed             224     init             */
 '\327', /*                             med              */
 '\327', /*                             final            */
 '\327', /*                             iso              */
 '\341', /* faa                 225     init             */
 '\341', /*                             med              */
 '\341', /*                             final            */
 '\341', /*                             iso              */
 '\362', /* quaaf               226     init             */
 '\362', /*                             med              */
 '\303', /*                             final            */
 '\303', /*                             iso              */
 '\356', /* kaaf                227     init             */
 '\356', /*                             med              */
 '\277', /*                             final            */
 '\277', /*                             iso              */
 '\357', /* laam                228     init             */
 '\357', /*                             med              */
 '\300', /*                             final            */
 '\300', /*                             iso              */
 '\360', /* meem                229     init             */
 '\360', /*                             med              */
 '\301', /*                             final            */
 '\301', /*                             iso              */
 '\361', /* noon                230     init             */
 '\361', /*                             med              */
 '\302', /*                             final            */
 '\302', /*                             iso              */
 '\344', /* haa                 231     init             */
 '\352', /*                             med              */
 '\273', /*                             final            */
 '\156', /*                             iso              */
 '\314', /* waw                 232     init             */
 '\314', /*                             med              */
 '\314', /*                             final            */
 '\314', /*                             iso              */
 '\306', /* alif maksura        233     init             */
 '\306', /*                             med              */
 '\167', /*                             final            */
 '\306', /*                             iso              */
 '\363', /* ya                  234     init             */
 '\363', /*                             med              */
 '\165', /*                             final            */
 '\304', /*                             iso              */

/* Laam Alif group fonts                                  */
 
 '\324', /* Alif After Laam                              */
 '\321', /* Alif Ham Laam                                */
 '\322', /* Alif Madda Laam                              */
 '\323', /* Hamza Alif Laam                              */
 
/* General arabic fonts                                   */
 
 '\256', /* Alif Tanween                                 */
 '\241', /* Ba Q                                         */
 '\242', /* Seen Q                                       */
 '\101', /* Bug char                                     */
 '\240', /* Space char                                   */
 '\327', /* Tamdeed                                      */
 '\243'  /* Tan Fat bq                                   */
                                                             
};
   
/* Enhanced Arabic Font Shapes */

unsigned char    arabenhance[] = 

{

/* Normal Diacritic font array -- DIAC_NORMAL            */

 '\141', /* tan_fatha                                    */
 '\142', /* tan_dham                                     */
 '\143', /* tan_kas                                      */
 '\147', /* fatha                                        */
 '\150', /* dhamma                                       */
 '\151', /* kasra                                        */
 '\153', /* shadda                                       */
 '\152', /* sukun                                        */
 '\101', /* char_bug                                     */
 '\101', /* char_bug                                     */
 '\101', /* char_bug                                     */
 '\144', /* shadda_fat                                   */
 '\145', /* shadda_dam                                   */
 '\146', /* shadda_kas                                   */

/* Medial Diacritic font array -- DIAC_MEDIAL            */

 '\373', /* fatha c                                      */
 '\374', /* dhamma_c                                     */
 '\375', /* kasra_c                                      */
 '\335', /* shadda_c                                     */
 '\376', /* sukun_c                                      */
 '\101', /* char_bug                                     */
 '\101', /* char_bug                                     */
 '\101', /* char_bug                                     */
 '\370', /* shadda_fat_c                                 */
 '\371', /* shadda_dham_c                                */
 '\372', /* shadda_kas_c                                 */

/* Baa Q Diacritic font array -- DIAC_BAA_Q              */

 '\243', /* tan_fat_bq                                   */
 '\244', /* tan_dham_bq                                  */
 '\245', /* tan_kas_bq                                   */
 '\251', /* tatha_bq                                     */
 '\252', /* dhamma_bq                                    */
 '\253', /* kas_bq                                       */
 '\255', /* shadda_bq                                    */
 '\254', /* sukun_bq                                     */
 '\101', /* char_bug                                     */
 '\101', /* char_bug                                     */
 '\101', /* char_bug                                     */
 '\246', /* shadda_fat_bq                                */
 '\247', /* shadda_dham_bq                               */
 '\250', /* shadda_kas_bq                                */

/* Seen Q Diacritic font array -- DIAC_SEEN_Q             */

 '\243', /* tan_fat_sq                                   */
 '\257', /* tan_dham_sq                                  */
 '\260', /* tan_kas_sq                                   */
 '\264', /* fatha_sq                                     */
 '\265', /* dhamma_sq                                    */
 '\266', /* kasra_sq                                     */
 '\270', /* shadda_sq                                    */
 '\267', /* sukun_sq                                     */
 '\101', /* char_bug                                     */
 '\101', /* char_bug                                     */
 '\101', /* char_bug                                     */
 '\261', /* shadda_fat_sq                                */
 '\262', /* shadda_dham_sq                               */
 '\263', /* shadda_kas_sq                                */
 
/* Miscellaneous char font array -- MISC_CHAR             */

 '\041', /* exclamation                                   */
 '\042', /* quote                                         */
 '\043', /* national money                                */
 '\044', /* international money                           */
 '\045', /* percent sign                                  */
 '\046', /* ampersand                                     */
 '\047', /* decimal comma                                 */
 '\051', /* open paren                                    */
 '\050', /* closing paren                                 */
 '\052', /* asterix                                       */
 '\053', /* plus sign                                     */
 '\054', /* arabic comma                                  */
 '\055', /* hypen                                         */
 '\056', /* period                                        */
 '\057', /* slant / solidus                               */
 '\060', /* india zero                                    */
 '\061', /* india one                                     */
 '\062', /* india two                                     */
 '\063', /* india three                                   */
 '\064', /* india four                                    */
 '\065', /* india five                                    */
 '\066', /* india six                                     */
 '\067', /* india seven                                   */
 '\070', /* india eight                                   */
 '\071', /* india nine                                    */
 '\072', /* colon                                         */
 '\073', /* arab semicolon                                */
 '\074', /* less than sign                                */
 '\075', /* equal sign                                    */
 '\076', /* greater than sign                             */
 '\077', /* question mark                                 */
 '\100', /* commerical at                                 */

/* Special char font array '\0 1'- SPEC_1                 */

 '\133', /* open bracket                                  */
 '\134', /* reverse slant                                 */
 '\135', /* close bracket                                 */
 '\136', /* up arrow                                      */
 '\137', /* underline                                     */

/* Special char font array n'\0 2'- SPEC_2                */

 '\173', /* open brace                                   */
 '\174', /* vertical line                                */
 '\175', /* close brace                                  */
 '\176', /* over line                                    */

/* Arabic mapping array: codar-u to font codes -- ARAB_MAP */ 
/*    font                      codar u                    */
/*    code   name               code    context            */

 '\140', /* hamza               193     init             */
 '\140', /*                             med              */
 '\140', /*                             final            */
 '\140', /*                             iso              */
 '\316', /* alif madda          194     init             */
 '\107', /*                             med              */
 '\107', /*                             final            */
 '\316', /*                             iso              */
 '\315', /* alif hamza          195     init             */
 '\106', /*                             med              */
 '\106', /*                             final            */
 '\315', /*                             iso              */
 '\307', /* waw hamza           196     init             */
 '\307', /*                             med              */
 '\307', /*                             final            */
 '\307', /*                             iso              */
 '\317', /* hamza under alif    197     init             */
 '\110', /*                             med              */
 '\110', /*                             final            */
 '\317', /*                             iso              */
 '\330', /* hamza on cursy      198     init             */
 '\330', /*                             med              */
 '\166', /*                             final            */
 '\305', /*                             iso              */
 '\320', /* alif                199     init             */
 '\164', /*                             med              */
 '\164', /*                             final            */
 '\320', /*                             iso              */
 '\336', /* baa                 200     init             */
 '\336', /*                             med              */
 '\336', /*                             final            */
 '\336', /*                             iso              */
 '\162', /* taa marbuta         201     init             */
 '\163', /*                             med              */
 '\163', /*                             final            */
 '\162', /*                             iso              */
 '\337', /* taa                 202     init             */
 '\337', /*                             med              */
 '\337', /*                             final            */
 '\337', /*                             iso              */
 '\340', /* thaa                203     init             */
 '\340', /*                             med              */
 '\340', /*                             final            */
 '\340', /*                             iso              */
 '\345', /* jeem                204     init             */
 '\353', /*                             med              */
 '\274', /*                             final            */
 '\157', /*                             iso              */
 '\346', /* ha                  205     init             */
 '\354', /*                             med              */
 '\275', /*                             final            */
 '\160', /*                             iso              */
 '\347', /* khaa                206     init             */
 '\355', /*                             med              */
 '\276', /*                             final            */
 '\161', /*                             iso              */
 '\310', /* daal                207     init             */
 '\310', /*                             med              */
 '\310', /*                             final            */
 '\310', /*                             iso              */
 '\311', /* thaal               208     init             */
 '\311', /*                             med              */
 '\311', /*                             final            */
 '\311', /*                             iso              */
 '\312', /* raa                 209     init             */
 '\312', /*                             med              */
 '\312', /*                             final            */
 '\312', /*                             iso              */
 '\313', /* zaa                 210     init             */
 '\313', /*                             med              */
 '\313', /*                             final            */
 '\313', /*                             iso              */
 '\364', /* seen                211     init             */
 '\364', /*                             med              */
 '\331', /*                             final            */
 '\331', /*                             iso              */
 '\365', /* sheen               212     init             */
 '\365', /*                             med              */
 '\332', /*                             final            */
 '\332', /*                             iso              */
 '\366', /* saad                213     init             */
 '\366', /*                             med              */
 '\333', /*                             final            */
 '\333', /*                             iso              */
 '\367', /* dhaad               214     init             */
 '\367', /*                             med              */
 '\334', /*                             final            */
 '\334', /*                             iso              */
 '\325', /* ta                  215     init             */
 '\325', /*                             med              */
 '\325', /*                             final            */
 '\325', /*                             iso              */
 '\326', /* tha                 216     init             */
 '\326', /*                             med              */
 '\326', /*                             final            */
 '\326', /*                             iso              */
 '\342', /* ayn                 217     init             */
 '\350', /*                             med              */
 '\271', /*                             final            */
 '\154', /*                             iso              */
 '\343', /* ghayn               218     init             */
 '\351', /*                             med              */
 '\272', /*                             final            */
 '\155', /*                             iso              */
 '\101', /* char_bug            219     init             */
 '\101', /*                             med              */
 '\101', /*                             final            */
 '\101', /*                             iso              */
 '\101', /* char_bug            065     init             */
 '\101', /*                             med              */
 '\101', /*                             final            */
 '\101', /*                             iso              */
 '\101', /* char_bug            221     init             */
 '\101', /*                             med              */
 '\101', /*                             final            */
 '\101', /*                             iso              */
 '\101', /* char_bug            222     init             */
 '\101', /*                             med              */
 '\101', /*                             final            */
 '\101', /*                             iso              */
 '\101', /* char_bug            223     init             */
 '\101', /*                             med              */
 '\101', /*                             final            */
 '\101', /*                             iso              */
 '\327', /* tamdeed             224     init             */
 '\327', /*                             med              */
 '\327', /*                             final            */
 '\327', /*                             iso              */
 '\341', /* faa                 225     init             */
 '\341', /*                             med              */
 '\341', /*                             final            */
 '\341', /*                             iso              */
 '\362', /* quaaf               226     init             */
 '\362', /*                             med              */
 '\303', /*                             final            */
 '\303', /*                             iso              */
 '\356', /* kaaf                227     init             */
 '\356', /*                             med              */
 '\277', /*                             final            */
 '\277', /*                             iso              */
 I_LAAM, /* laam                228     init             */
 M_LAAM, /*                             med              */
 '\300', /*                             final            */
 '\300', /*                             iso              */
 '\360', /* meem                229     init             */
 '\360', /*                             med              */
 '\301', /*                             final            */
 '\301', /*                             iso              */
 '\361', /* noon                230     init             */
 '\361', /*                             med              */
 '\302', /*                             final            */
 '\302', /*                             iso              */
 '\344', /* haa                 231     init             */
 '\352', /*                             med              */
 '\273', /*                             final            */
 '\156', /*                             iso              */
 '\314', /* waw                 232     init             */
 '\314', /*                             med              */
 '\314', /*                             final            */
 '\314', /*                             iso              */
 '\306', /* alif maksura        233     init             */
 '\306', /*                             med              */
 '\167', /*                             final            */
 '\306', /*                             iso              */
 '\363', /* ya                  234     init             */
 '\363', /*                             med              */
 '\165', /*                             final            */
 '\304', /*                             iso              */

/* Laam Alif group fonts                                  */
 
 '\324', /* Alif After Laam                              */
 '\321', /* Alif Ham Laam                                */
 '\322', /* Alif Madda Laam                              */
 '\323', /* Hamza Alif Laam                              */
 
/* General arabic fonts                                   */
 
 '\256', /* Alif Tanween                                 */
 '\241', /* Ba Q                                         */
 '\242', /* Seen Q                                       */
 '\101', /* Bug char                                     */
 '\240', /* Space char                                   */
 '\327', /* Tamdeed                                      */
 '\243'  /* Tan Fat bq                                   */
                                                             
};
