/*  $Revision: 64.1 $  */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>

#define FONTID	89			/*  font format ID  */
#define MAXROW	150			/*  max row of character  */
#define MAXCOL	20			/*  max column of character(Byte)  */
#define MAXCHAR 255			/*  max character code */

unsigned char *buff,*buff_p,*max_p;		/*  font file buffer */
unsigned char (*raster)[MAXCOL],*raster_p;	/*  raster data buffer  */
unsigned char buf_land[MAXCOL][MAXROW];

int font_number;		/*  font number  */

/* font description  */

unsigned int design_size;
unsigned int check_sum;
unsigned int hppp;		/*  horizontally ratios of pixels per point  */
unsigned int vppp;		/*  vertically   ratios of pixels per point  */
unsigned int scalednumber;
unsigned int orientation;	/*  orientation (0:portrait 1:landscape)  */
unsigned int spacing;		/*  spacing (0:fixed 1:proportional)  */

/*  character description  */

struct char_description {
	unsigned int color;		/*  first run count Black or White */
	unsigned int char_code;		/*  character code  */
	unsigned int tfm_width;
	unsigned int dyn_f;		/*  dyn_f value  */
	unsigned int h_escape;		/*  horizontal escapement  */
	unsigned int v_escape;		/*  vertical escapement  */
	unsigned int width;
	unsigned int height;
	int h_offset;			/*  horizontal offset  */
	int v_offset;			/*  vertical offset  */
	unsigned char *raster_top;	/*  raster data top address  */
} char_descriptor [MAXCHAR + 1], *desc_p;

int max_width;			/*  max cell width  */
int max_up;			/*  max from baseline to cell top */
int max_down;			/*  max from baseline to cell under */
unsigned int repeat_count;

/*  flags  */

int bit_pos;			/*  current bit position for reading  */
int firstnybble;		/*  1st or 2nd nybble reading in a word  */

char *cmdname;			/*  command name of this command  */
char *fname;			/*  input file name  */
char *usage="usage: %s {-l,-p,-n} filename\n";
FILE *outfp=stdout;		/*  output stream  */

/********************
 *	main
 ********************/

main(argc,argv)
	int argc;
	char **argv;
{
	int i;
	int c;
	extern char *optarg;
	extern int optind;

	cmdname = argv[0];

	orientation = spacing = font_number = 0;
	fname = NULL;

	while ((c = getopt(argc,argv,"n:lp")) != EOF) {
		switch (c) {
		case 'l' : orientation = 1;
			   break;
		case 'p' : spacing = 1;
			   break;
		case 'n' : font_number = atoi(optarg);
			   break;
		case '?' : fprintf(stderr,usage,cmdname);
			   exit(1);
		}
	}
	fname = argv[optind];

	if ( fname == NULL ) {
		fprintf(stderr,usage,cmdname);
		exit(1);
	}

	if ((raster = (unsigned char (*)[MAXCOL])
		(raster_p = (unsigned char *) malloc(MAXROW * MAXCOL)))
								== NULL) {
		error("cannot get output buffer");
	}

	readfile();
	command_decode();
	down_descriptor();
	down_character();

	free(buff);
	free(raster);
}

readfile()
{
	struct stat stat_buf;
	unsigned int size;
	int fd;

	if ((fd=open(fname,O_RDONLY)) == -1) {
		error("cannot open font file");
	}
	if (fstat(fd,&stat_buf) == -1) {
		error("cannot read font file");
	}
	size = stat_buf.st_size;
	if ((buff = (unsigned char *)malloc(size)) == NULL) {	
		error("cannot get input buffer");
	}
	if (read(fd,buff,size) != size) {
		error("cannot read input file");
	}
	buff_p = buff;
	max_p = &buff[size-1];
	close(fd);
	return;
}

/****************************
 *	commans in font file
 ****************************/

/*  get nByte from input buffer  and fill them into a word  */

unsigned int get_nbyte( bytes )
	int bytes;
{
	unsigned int val;
	int i;

	val = 0;
	for (i=0; i<bytes; i++) {
		val <<= 8;
		val += *buff_p++;
	}
	return( val );
}

command_decode ()
{
	unsigned int cmd;

	preamble();
	
	desc_p = char_descriptor;
	desc_p->raster_top = NULL;
	max_width = max_up = max_down = 0;

	while ( buff_p <= max_p )
	    switch ( (int)(cmd = *buff_p++) ) {
		case  240 :
		case  241 :
		case  242 :
		case  243 :		/* special command */
			    buff_p += get_nbyte( cmd - 239 );
			    break;
		case  244 : 		/* numspecial command */
			    scalednumber = get_nbyte(4);	
			    break;
		case  245 : return;
		case  247 : error("bad font file");
		case  246 :
		case  248 :
		case  249 :
		case  250 :
		case  251 :
		case  252 :
		case  253 :
		case  254 :
		case  255 : break;
		default   : character( cmd );
			    break;
	    }
}

/*  get preamble  */

preamble()
{
	int i,len;


	while (*buff_p++ != 247)		/*  find a preamble  */
		if (buff_p > max_p)	error("bad font file");
	if ( *buff_p++ != FONTID )	error("bad font file");
	len = *buff_p++;
	for (i=0; i<len; i++)	buff_p++;	/*  skip a comment */
	design_size = get_nbyte(4);
	check_sum = get_nbyte(4);
	hppp = get_nbyte(4);
	vppp = get_nbyte(4);

	return;
}

/*  read character data  */

character( flag_byte )
	unsigned int flag_byte;
{
	unsigned int format;		/*  char descriptor format  */
	unsigned int length;		/*  packet length of this char */
	unsigned char *next;		/*  next char data address  */

/*  character preamble  */

	format = flag_byte & 0x7;
	if ( flag_byte & 0x8 )
		desc_p->color = 1;
	  else
		desc_p->color = 0;
	desc_p->dyn_f = ( flag_byte >> 4 ) & 0xF;

	switch( format ) {
	    case 0 :
	    case 1 :
	    case 2 :
	    case 3 :
		length = *buff_p++ + (format << 8);	/* short format */
		desc_p->char_code = *buff_p++;
		next = buff_p + length;
		desc_p->tfm_width = get_nbyte(3);
		desc_p->h_escape = *buff_p++;
		desc_p->v_escape = 0;
		desc_p->width = *buff_p++;
		desc_p->height = *buff_p++;
		desc_p->h_offset = (int)*((char *)buff_p)++;
		desc_p->v_offset = (int)*((char *)buff_p)++;
		break;
	    case 4 :
	    case 5 :
	    case 6 :				/* extended short format */
		length = get_nbyte(2) + ( (format-4) << 16);
		desc_p->char_code = *buff_p++;
		next = buff_p + length;
		desc_p->tfm_width = get_nbyte(3);
		desc_p->h_escape = get_nbyte(2);
		desc_p->v_escape = 0;
		desc_p->width = get_nbyte(2);
		desc_p->height = get_nbyte(2);
		desc_p->h_offset = (int) get_nbyte(2);
		desc_p->v_offset = (int) get_nbyte(2);
		break;
	    case 7 :				/* long format */
		length = get_nbyte(4);
		desc_p->char_code = get_nbyte(4);
		next = buff_p + length;
		desc_p->tfm_width = get_nbyte(4);
		desc_p->h_escape = get_nbyte(4) / 65536;
		desc_p->v_escape = get_nbyte(4) / 65536;
		desc_p->width = get_nbyte(4);
		desc_p->height = get_nbyte(4);
		desc_p->h_offset = (int) get_nbyte(4);
		desc_p->v_offset = (int) get_nbyte(4);
		break;
	}

/*  to find max sizes of cells */
	if (desc_p->h_escape > max_width)	max_width = desc_p->h_escape;

	if ((int)desc_p->height - desc_p->v_offset > max_down)
		max_down = (int)desc_p->height - desc_p->v_offset;
	if (desc_p->v_offset > max_up)
		max_up = desc_p->v_offset;

	desc_p->raster_top = buff_p;
	(++desc_p)->raster_top = NULL;
	buff_p = next;

	return;
}

/**************************************
 *	get raster datas from buffer
 **************************************/

/*  get a 1bit from buffer  */

unsigned int get_bit()
{
	unsigned int tmp;

	tmp = (*buff_p >> bit_pos) & 1;
	if ( --bit_pos < 0 ) {
		bit_pos = 7;
		buff_p++;
	}
	return(tmp);
}

/*  get simple raster data  */

get_simple_raster()
{
	int row,bit,j;

	for (row=0; row<desc_p->height; row++) {
		bit = 7;
		*( raster_p = &raster[row][0] ) = 0;
		for (j=0; j<desc_p->width; j++) {
			*raster_p |= get_bit() << bit;
			if ( --bit < 0 ) {
				bit = 7;
				*++raster_p = 0;
			}
		}
	}
}

/*  get nybble(4bits) from buffer  */

unsigned int get_nybble()
{
	if ( firstnybble ) {
		firstnybble = 0;
		return( (*buff_p >> 4) & 0xF );
	} 
	else {
		firstnybble = 1;
		return( *buff_p++ & 0xF );
	}
}

/*  unpack a nybble data  */

unpack()
{
	int val,i;

	val = get_nybble();
	switch ( val ) {
	    case 15 :
		repeat_count = 1;
		return( unpack() );
	    case 14 :
		repeat_count = unpack();
		return( unpack() );
	    case 0 :
		i = 1;
		while( !(val = get_nybble()) ) i++;
		while ( i ) {
			val = val * 16 + get_nybble();
			i--;
		}
		return( val - 15 + (13 - desc_p->dyn_f)*16 + desc_p->dyn_f );
	    default :
		if ( val <= desc_p->dyn_f )  return( val );
		else
			return( (val - desc_p->dyn_f -1)*16
					+ get_nybble() + desc_p->dyn_f +1);
	}
}

/*  get packed raster data from buffer */

get_packed_raster()
{
	int column,row,bit,run_count,color,j,k;
	unsigned  char *p1, *p2;

	color = desc_p->color;
	row = 0;
	run_count = unpack();
	while ( row < desc_p->height ) {
		column = 0;
		bit = 7;
		*(raster_p = &raster[row][0]) = 0;
		while ( column < desc_p->width ) {
			if ( !run_count ) {
				run_count = unpack();
				if ( color )
					color = 0;
				else
					color = 1;
			}
			*raster_p |= color << bit;
			if ( --bit < 0 ) {
				bit = 7;
				*++raster_p = 0;
			}
			run_count--;
			column++;
		}
		for ( j=repeat_count; j>0; j--) {
			p1 = &raster[row][0];
			p2 = &raster[++row][0];
			for ( k=0; k<desc_p->width; k++ )  *p2++ = *p1++;
			repeat_count = 0;
		}
		row++;
	}
}

/*************************
 *      font download
 *************************/

#define Put2bytes(x)	tmp=x; fwrite((char *)&tmp,2,1,outfp)

/* download a font header */

down_descriptor()
{
	short tmp;

	if ( orientation ) fprintf(outfp,"\033&l1O");
	fprintf(outfp,"\033*c0F");			/* delete all font */

	fprintf(outfp,"\033*c%dD",font_number);		/* assign font id */
	fprintf(outfp,"\033)s26W");			/* create font */

	Put2bytes(26);				/* font descriptor size */
	Put2bytes(1);				/* font type (8-bit) */
	Put2bytes(0);				/* */
	Put2bytes(max_up);			/* baseline distance */
	if ( !orientation ) {	/* portrait */
		Put2bytes(max_width);			/* cell width */
		Put2bytes(max_up + max_down);		/* cell height */
		Put2bytes( (orientation << 8) + spacing );
		Put2bytes(277);				/* symbol set */
		Put2bytes(max_width * 4);		/* pitch */
		Put2bytes( (max_up + max_down) * 4);	/* height */
		Put2bytes(0);				/* xHeight */
	} else {		/* landscape */
		Put2bytes(max_width);			/* cell width */
		Put2bytes(max_up + max_down);		/* cell height */
		Put2bytes( (orientation << 8) + spacing );
		Put2bytes(277);				/* symbol set */
		Put2bytes(max_width * 4);		/* pitch */
		Put2bytes( (max_up + max_down) * 4);	/* height */
		Put2bytes(0);				/* xHeight */
	}
	Put2bytes(0);		/* width type (normal) + style (upright) */
	Put2bytes(5);		/* stroke weight (midium) + typeface(TmsRmn) */

	fflush(outfp);

	return;
}

/*  decode character datas and download them */

down_character()
{
	desc_p = char_descriptor;
	while ( (buff_p = desc_p->raster_top) != NULL ) {
		bit_pos = 7;
		firstnybble = 1;
		repeat_count = 0;

		if (desc_p->dyn_f == 14)
			get_simple_raster();
		else
			get_packed_raster();
		if ( !orientation )
			down_raster();
		else
			down_landscape();

		desc_p++;
	}
	fprintf(outfp,"\033*c5F");		/*  make font primary */
	fprintf(outfp,"\033(%dX",font_number);	/*  select font with id */

	fflush(outfp);
	return;
}
	
/*  download raster data  */	

down_raster()
{
	int i,byte_width;
	short tmp;

	if ( desc_p->width % 8 == 0 )
		byte_width = desc_p->width / 8;
	else
		byte_width = (desc_p->width / 8) + 1;
/* character header */
	fprintf(outfp,"\033*c%dE",desc_p->char_code);	/* character code */
	fprintf(outfp,"\033(s%dW",byte_width*desc_p->height+16);
					/*  byte size of descriptor and data */

	Put2bytes(0x0400);	/* format + continuation */
	Put2bytes(0x0E01);	/* descriptor size + class */
	Put2bytes(orientation << 8);	/* orientation + X */
	Put2bytes( -desc_p->h_offset );	/* left offset */
	Put2bytes( desc_p->v_offset );	/* top offset */
	Put2bytes( desc_p->width );	/* character width */
	Put2bytes( desc_p->height );	/* character height */
	Put2bytes( desc_p->h_escape * 4);	/* delta X */

/* binary data */
	for (i=0; i<desc_p->height; i++)
		fwrite(raster[i],byte_width,1,outfp);

	fflush(outfp);

	return;
}

/*  download raster data as landscape  */	

#define get_raster(i,j)	\
	(*( &raster[0][0] + i*MAXCOL + (j/8)) >> (7-j%8)) & 1

down_landscape()
{
	int i,j,byte_width,byte_height,bit,bitpos;
	short tmp;
	unsigned char *land_p;

	byte_width = (desc_p->width + 7) / 8;
	byte_height = (desc_p->height + 7) / 8;

	land_p = &buf_land[0][0];
	for (i=0; i < desc_p->width; i++) {
		*( land_p = (unsigned char *)buf_land + (MAXROW * i) ) = 0;
		bitpos = 7;
		for (j=0; j < desc_p->height; j++) {
			bit = get_raster( j, (desc_p->width - i - 1));
			*land_p |= (bit << bitpos);
			if (bitpos-- == 0) {
				*(++land_p) = 0;
				bitpos = 7;
			}
		}
	}
			
/* character header */
	fprintf(outfp,"\033*c%dE",desc_p->char_code);	/* character code */
	fprintf(outfp,"\033(s%dW",byte_height*desc_p->width+16);
					/*  byte size of descriptor and data */

	Put2bytes(0x0400);	/* format + continuation */
	Put2bytes(0x0E01);	/* descriptor size + class */
	Put2bytes(orientation << 8);	/* orientation + X */
	Put2bytes(  - desc_p->v_offset );	/* left offset */
	if ( desc_p->h_offset == 0 ) {
		Put2bytes( desc_p->width - 1 );		/* top offset */
	} else {
		Put2bytes( desc_p->width - desc_p->h_offset ); /* top offset */
	}
	Put2bytes( desc_p->height );	/* character width */
	Put2bytes( desc_p->width );	/* character height */
	Put2bytes( desc_p->h_escape * 4);	/* delta X */

/* binary data */
	for (i=0; i<desc_p->width; i++)
		fwrite(buf_land[i],byte_height,1,outfp);

	fflush(outfp);

	return;
}

/*   error display  */

error(msg)
	char *msg;
{
	fprintf(stderr,"%s :%s\n",cmdname,msg);
	exit(2);
}

