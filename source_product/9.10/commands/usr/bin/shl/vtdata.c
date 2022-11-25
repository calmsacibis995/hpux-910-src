/* @(#) $Revision: 29.1 $ */   
#include <netio.h>

/* Due to a Series 500 linker bug, I need to initialize this buffer */

struct fis _vtfisbuf = {0,0};
