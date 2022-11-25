# /* file xprint.c */
# /* SCCS	xprint.c	REV(9.1);	DATE(11/07/89	14:36:17) */
# /* KLEENIX_ID @(#)xprint.c	10.1 90/02/05 */
/*
 * xprint.c - display PCC tree in an X window
 *
 * Directions:
 *    To display PCC trees, link in this file and /usr/lib/libX.a
 *    Call the routine "xtprint" with the name of the NODE* as its
 *    only argument.
 *        There are currently two mouse buttons defined:
 *           right - toggles between display modes
 *           left  - exits the window.
 *
 *    There are several configurable parameters that can be set in ~/.Xdefaults
 *        xprint.BigFont       - The font used to display node names.
 *        xprint.LittleFont    - The font used for other information.
 *        xprint.Margin        - # of pixels between the tree and the edge of
 *                               the window.
 *        xprint.BorderWidth   - Width in pixels
 *        xprint.NodeMargin    - # of pixels between the node text and the edge
 *                               of the node boundary.
 *        xprint.NodeBorderWidth - Width in pixels.
 *        xprint.NodeHSpace    - Minimal horizonal seperation between nodes.
 *        xprint.NodeVSpace    - Minimal vertical seperation between nodes.
 *
 * Problems:
 *   Known problems/deficiencies include:
 *      1. I've seen a rapid sucession of Xevents (buttons/exposure) hang
 *         the window.
 *      2. There is no known way of saving a window on the screen.
 *      3. Windows may be too large to fit on the screen.
 *      4. There is a relatively small number of strings displayed in the full
 *         mode.  Modify the routine "strings_xtree" to get other strings.
 *
 * Author: mev       3/20/88
 */
#ifdef XPRINT
#include <X/Xlib.h>
#include "mfile2"
flag treedebug = 0;

/* defaults */
#define BIGFONT "12x20b"
#define LITTLEFONT "6x10"
#define MARGIN 20
#define BORDERWIDTH 5
#define NODEBORDERWIDTH 2
#define NODEMARGIN 4
#define NODEHSPACE 12
#define NODEVSPACE 30

static char *bigfont = BIGFONT;
static char *littlefont = LITTLEFONT;
static int margin = MARGIN;
static int borderwidth = BORDERWIDTH;
static int nodeborderwidth = NODEBORDERWIDTH;
static int nodemargin = NODEMARGIN;
static int nodehspace = NODEHSPACE;
static int nodevspace = NODEVSPACE;
static char *tnames[] = {
  "undef",
  "farg",
  "char",
  "short",
  "int",
  "long",
  "float",
  "double",
  "strty",
  "unionty",
  "enumty",
  "moety",
  "uchar",
  "ushort",
  "unsigned",
  "ulong",
  "void",
  "labty",
  "tnull",
  "?","?" };


static FontInfo *bigfontinfo;
static FontInfo *littlefontinfo;
static int fulldisplay = 1;

static Pixmap gray_pixmap;
static short gray_bits[16] = {
  0xaaaa,0x5555,0xaaaa,0x5555,
  0xaaaa,0x5555,0xaaaa,0x5555,
  0xaaaa,0x5555,0xaaaa,0x5555,
  0xaaaa,0x5555,0xaaaa,0x5555
  };

static Pixmap gray2_pixmap;
static short gray2_bits[16] = {
  0xcccc,0x3333,0xcccc,0x3333,
  0xcccc,0x3333,0xcccc,0x3333,
  0xcccc,0x3333,0xcccc,0x3333,
  0xcccc,0x3333,0xcccc,0x3333
  };

static Pixmap stripe_pixmap;
static short stripe_bits[16] = {
  0xFFFF,0x0,0xFFFF,0x0,
  0xFFFF,0x0,0xFFFF,0x0,
  0xFFFF,0x0,0xFFFF,0x0,
  0xFFFF,0x0,0xFFFF,0x0
  };

static Pixmap stripe2_pixmap;
static short stripe2_bits[16] = {
  0xFFFF,0xFFFF,0x0,0x0,
  0xFFFF,0xFFFF,0x0,0x0,
  0xFFFF,0xFFFF,0x0,0x0,
  0xFFFF,0xFFFF,0x0,0x0
  };

static Pixmap stripe4_pixmap;
static short stripe4_bits[16] = {
  0xFFFF,0xFFFF,0xFFFF,0xFFFF,
  0x0,0x0,0x0,0x0,
  0xFFFF,0xFFFF,0xFFFF,0xFFFF,
  0x0,0x0,0x0,0x0
  };

static Pixmap dot_pixmap;
static short dot_bits[16] = {
  0x0,0x606,0x606,0x0,
  0x0,0x6060,0x6060,0x0,
  0x0,0x606,0x606,0x0,
  0x0,0x6060,0x6060,0x0
  };

typedef struct tdat TDAT;
struct tdat {
  NODE *pnode;
  Window w;
  TDAT *left;
  TDAT *right;
  char *str;
  int numstr;
  int wid;
  int wid_max;
  int wid_offset;
  int hgt;
  int hgt_max;
  int hgt_offset;
};

/*
 * error - print error routines
 */

static error(fmt,arg1,arg2) {
  fprintf(stderr,fmt,arg1,arg2);
}

/*
 * create_xtree - builds up tree data structure, from the nodes.
 */

static TDAT *create_xtree(p) NODE *p; {
  TDAT *td = (TDAT *) malloc(sizeof(TDAT));
  short opty = optype(p->in.op);
  td->pnode = p;
  td->w = 0;
  td->left = 0;
  td->right = 0;
  td->str = 0;
  if (opty != LTYPE)  td->left  = create_xtree(p->in.left);
  if (opty == BITYPE) td->right = create_xtree(p->in.right);
  return(td);
}

static free_xtree(t) TDAT *t; {
  if (t->str) { 
    free(t->str);
    t->str = 0;
  }
  if (t->left)  free_xtree(t->left);
  if (t->right) free_xtree(t->right);
  free(t);
}

/*
 * place_xtree - set the positions of the subwindows
 */

static place_xtree(t,w,h,hincr,hspace) TDAT *t; int w,h,hincr,hspace; {
  t->hgt_offset = h;
  if (t->left && t->right) {
    place_xtree(t->left,w,h+hincr+hspace,hincr,hspace);
    place_xtree(t->right,w+t->left->wid_max+nodehspace,h+hincr+hspace,hincr,hspace);
    t->hgt_max = 
      (t->left->hgt_max > t->right->hgt_max) ?
	t->left->hgt_max : t->right->hgt_max;
  } else if (t->left) {
    place_xtree(t->left,w,h+hincr+hspace,hincr,hspace);
    t->hgt_max = t->left->hgt_max;
  } else {
    t->wid_offset = w;
    t->hgt_max = h+hincr;
  }
  t->wid_offset = w + (t->wid_max - t->wid)/2;
}

/*
 * strings_xtree - draw the sub-window
 */

static strings_xtree(t) TDAT *t; {
  char buf[400];
  char *p,*tp;
  TTYPE typ;
  p = buf;
  t->wid = 0;
  t->hgt = 0;

  /* Get rid of the old strings */
  if (t->str) free(t->str);

  /* find the new strings required */
  sprintf(p,"%s",opst[t->pnode->in.op]); /* node name */
  t->numstr = 1;
  t->hgt += bigfontinfo->height;
  if (t->wid < XStringWidth(p,bigfontinfo,0,0))
    t->wid = XStringWidth(p,bigfontinfo,0,0);
  p += strlen(p) + 1;

  sprintf(p,"0x%x)",t->pnode);
  t->numstr++;
  if (fulldisplay)
  {
    t->hgt += littlefontinfo->height+2;
    if (t->wid < XStringWidth(p,littlefontinfo,0,0))
      t->wid = XStringWidth(p,littlefontinfo,0,0);
  }
  p += strlen(p) + 1;

#if 0
  for (tp = p, typ=t->pnode->in.type;;typ=DECREF(typ)) {
    if (ISPTR(typ)) sprintf(tp,"PTR ");
    else if (ISFTN(typ)) sprintf(tp,"FTN ");
    else if (ISARY(typ)) sprintf(tp,"ARY ");
    else if (typ >= UNDEF && typ <= TNULL) 
    {
      sprintf(tp,"%s",tnames[typ]);
      break;
    }
    tp += strlen(tp);
  }
#endif
  t->numstr++;
  if (fulldisplay)
  {
    t->hgt += littlefontinfo->height+2;
    if (t->wid < XStringWidth(p,littlefontinfo,0,0))
      t->wid = XStringWidth(p,littlefontinfo,0,0);
  }
  p += strlen(p) + 1;

  /* malloc space, and copy more strings */
  t->str = (char *) malloc(p-buf);
  memcpy(t->str,buf,p-buf);

  /* add borders */
  t->wid += (nodemargin*2);
  t->hgt += (nodemargin*2);

  t->wid_max = 0;
  t->hgt_max = t->hgt;
  if (t->left) {
    strings_xtree(t->left);
    t->wid_max = t->left->wid_max;
    if (t->left->hgt_max > t->hgt_max)
      t->hgt_max = t->left->hgt_max;
  }
  if (t->right) {
    strings_xtree(t->right);
    t->wid_max += t->right->wid_max + nodehspace;
    if (t->right->hgt_max > t->hgt_max)
      t->hgt_max = t->right->hgt_max;
  }
  if (t->wid > t->wid_max)
    t->wid_max = t->wid;
}

/*
 * cwin - create windows for this node and its descendants
 */
static cwin_xtree(t,win) TDAT *t; Window win; {
  t->w = XCreateWindow(win,t->wid_offset,t->hgt_offset,t->wid,t->hgt,nodeborderwidth,BlackPixmap,WhitePixmap);
  if (t->left) cwin_xtree(t->left,win);
  if (t->right) cwin_xtree(t->right,win);
}

/*
 * display_xtree - generate sequence to display this node and its descendants
 */

static display_xtree(t,win) TDAT *t; Window win; {
  char *p = t->str;
  int i;
  int ht = nodemargin;
  if (fulldisplay) {
    XText(t->w,nodemargin,nodemargin,p,strlen(p),bigfontinfo->id,BlackPixel,WhitePixel);
    ht = nodemargin + bigfontinfo->height + 2;
    for (i=1;i<t->numstr;i++) {
      p += strlen(p) + 1;
      XText(t->w,nodemargin,ht,p,strlen(p),littlefontinfo->id,BlackPixel,WhitePixel);
      ht += littlefontinfo->height+2;
    }
  } else {
    switch(BTYPE(t->pnode->in.type)) {
    case CHAR:
    case SHORT:
    case INT:
    case LONG:
      XChangeBackground(t->w,WhitePixmap);
      break;
    case FLOAT:
      XChangeBackground(t->w,gray2_pixmap);
      break;
    case DOUBLE:
      XChangeBackground(t->w,gray_pixmap);
      break;
    case STRTY:
      XChangeBackground(t->w,stripe_pixmap);
      break;
    case UNIONTY:
      XChangeBackground(t->w,stripe2_pixmap);
      break;
    case ENUMTY:
    case MOETY:
      XChangeBackground(t->w,stripe4_pixmap);
      break;
    case UCHAR:
    case USHORT:
    case UNSIGNED:
    case ULONG:
      XChangeBackground(t->w,dot_pixmap);
      break;
    case VOID:
      XChangeBackground(t->w,BlackPixmap);
      break;
    default: 
      break;
    }
    XClear(t->w);
    XText(t->w,nodemargin,nodemargin,p,strlen(p),bigfontinfo->id,BlackPixel,WhitePixel);
  }
  if (t->left)  {
    display_xtree(t->left,win);
    XLine(win,
	  t->wid_offset+t->wid/2,t->hgt_offset+t->hgt,
	  t->left->wid_offset+t->left->wid/2,t->left->hgt_offset,
	  1,1,
	  BlackPixel,
	  GXclear,
	  AllPlanes);
  }
  if (t->right) {
    display_xtree(t->right,win);
    XLine(win,
	  t->wid_offset+t->wid/2,t->hgt_offset+t->hgt,
	  t->right->wid_offset+t->right->wid/2,t->right->hgt_offset,
	  1,1,
	  BlackPixel,
	  GXclear,
	  AllPlanes);
  }
}

/*
 * input_loop - loop to sit requesting events
 */

static input_loop(t,win) TDAT *t; Window win; {
  static XButtonEvent event;
  int async = 0;
  XSelectInput(win,ButtonPressed|ButtonReleased|ExposeRegion|ExposeWindow);
  while(1) {
    XNextEvent(&event);
    switch(event.type) {
    case ExposeRegion:
    case ExposeWindow:
      display_xtree(t,win);
      XFlush();
      break;
    case ButtonPressed:
      break;
    case ButtonReleased:
      switch(event.detail & 0xFF)
      {
      case 0: /* RIGHT */
	XDestroyWindow(win);
	fulldisplay = (1-fulldisplay);
	strings_xtree(t);
	place_xtree(t,margin,margin,t->hgt_max,nodevspace);
	win = XCreateWindow(RootWindow,0,0,t->wid_max+2*margin,t->hgt_max+margin,borderwidth,BlackPixmap,WhitePixmap);
	cwin_xtree(t,win);
	XMapSubwindows(win);
	XMapWindow(win);
	display_xtree(t,win);
	XFlush();
	XSelectInput(win,ButtonPressed|ButtonReleased|ExposeRegion|ExposeWindow);
	break;
      case 1: /* MIDDLE */
	break;
      case 2: /* LEFT */
	XDestroyWindow(win);
	XSync();
	free_xtree(t);
	return;
      }
      break;
    }
  }
}
  

/*
 * xtinit - initialize the X window stuff
 */

static xtinit() {
  char *p;
  XOpenDisplay(0);
  gray_pixmap = XMakePixmap(XStoreBitmap(16,16,gray_bits),BlackPixel,WhitePixel);
  gray2_pixmap = XMakePixmap(XStoreBitmap(16,16,gray2_bits),BlackPixel,WhitePixel);
  stripe_pixmap = XMakePixmap(XStoreBitmap(16,16,stripe_bits),BlackPixel,WhitePixel);
  stripe2_pixmap = XMakePixmap(XStoreBitmap(16,16,stripe2_bits),BlackPixel,WhitePixel);
  stripe4_pixmap = XMakePixmap(XStoreBitmap(16,16,stripe4_bits),BlackPixel,WhitePixel);
  dot_pixmap = XMakePixmap(XStoreBitmap(16,16,dot_bits),BlackPixel,WhitePixel);
  /* load the defaults */
  if ((p = XGetDefault("xprint","BigFont")) == NULL) p = BIGFONT;
  if ((bigfontinfo = XOpenFont(p)) == NULL) error("can't open font %s",p);
  if ((p = XGetDefault("xprint","LittleFont")) == NULL) p = LITTLEFONT;
  if ((littlefontinfo = XOpenFont(p)) == NULL) error("can't open font %s",p);
  if (((p = XGetDefault("xprint","Margin")) == NULL) ||
      (sscanf(p,"%d",&margin) != 1)) margin = MARGIN;
  if (((p = XGetDefault("xprint","BorderWidth")) == NULL) ||
      (sscanf(p,"%d",&borderwidth) != 1)) borderwidth = BORDERWIDTH;
  if (((p = XGetDefault("xprint","NodeBorderWidth")) == NULL) ||
      (sscanf(p,"%d",&nodeborderwidth) != 1)) nodeborderwidth = NODEBORDERWIDTH;
  if (((p = XGetDefault("xprint","NodeMargin")) == NULL) ||
      (sscanf(p,"%d",&nodemargin) != 1)) nodemargin = NODEMARGIN;
  if (((p = XGetDefault("xprint","NodeHSpace")) == NULL) ||
      (sscanf(p,"%d",&nodehspace) != 1)) nodehspace = NODEHSPACE;
  if (((p = XGetDefault("xprint","NodeVSpace")) == NULL) ||
      (sscanf(p,"%d",&nodevspace) != 1)) nodevspace = NODEVSPACE;
}

/*
 * xtprint - display pcc node in an X window
 */

void xtprint(p) NODE *p; {
  extern int free();
  static int init = 1;
  TDAT *t;
  Window win;
  if (init) { xtinit(); init = 0; }
  /* create the initial tree */
  t = create_xtree(p);

  /* initialize the sub-windows */
  strings_xtree(t);
  place_xtree(t,margin,margin,t->hgt_max,nodevspace);
  win = XCreateWindow(RootWindow,0,0,t->wid_max+2*margin,t->hgt_max+margin,borderwidth,BlackPixmap,WhitePixmap);
  cwin_xtree(t,win);
  XMapSubwindows(win);
  XMapWindow(win);
  display_xtree(t,win);
  XFlush();
  input_loop(t,win);
}
#endif
