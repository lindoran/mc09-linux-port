/*
 * 6809 Cross Assembler
 *
 * ?COPY.TXT 1983-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "xasm.h"

static int  compsym(unsigned char *,unsigned char *);
static bool locins(unsigned char []);
static bool readline(void);
static bool isterm(char);
static bool issymbol(char);
static int  evlind(unsigned char);
static int  memop(void);
static int  addreg(void);
static int  eval(void);
static int  getval(void);
static void reperr(int);
static int  getreg(void);
static int  nxtelem(void);
static void wrrec(void);
static void write_title(void);
static void optfail(void);
static char chupper(char);

/* 6809 opcode table */
static unsigned char const opcodes[] = {
	'L','B','S','R',0x88,0x17,0xcf,0xcf,0xcf,
	'L','D','A',	0x82,0x86,0x96,0xa6,0xb6,
	'C','M','P','A',0x82,0x81,0x91,0xa1,0xb1,
	'B','N','E',	0x86,0x26,0xcf,0xcf,0xcf,
	'B','E','Q',	0x86,0x27,0xcf,0xcf,0xcf,
	'B','R','A',	0x86,0x20,0xcf,0xcf,0xcf,
	'L','D','X',	0x83,0x8e,0x9e,0xae,0xbe,
	'L','D','B',	0x82,0xc6,0xd6,0xe6,0xf6,
	'L','D','D',	0x83,0xcc,0xdc,0xec,0xfc,
	'S','T','A',	0x82,0xcf,0x97,0xa7,0xb7,
	'S','T','B',	0x82,0xcf,0xd7,0xe7,0xf7,
	'S','T','D',	0x83,0xcf,0xdd,0xed,0xfd,
	'S','T','X',	0x83,0xcf,0x9f,0xaf,0xbf,
	'D','E','C','A',0x81,0x4a,0,0,0,
	'D','E','C','B',0x81,0x5a,0,0,0,
	'I','N','C','A',0x81,0x4c,0,0,0,
	'I','N','C','B',0x81,0x5c,0,0,0,
	'L','D','Y',	0x84,0x8e,0x9e,0xae,0xbe,
	'L','B','R','A',0x88,0x16,0xcf,0xcf,0xcf,
	'P','U','L','S',0x89,0x35,0xcf,0xcf,0xcf,
	'P','U','L','U',0x89,0x37,0xcf,0xcf,0xcf,
	'P','S','H','S',0x89,0x34,0xcf,0xcf,0xcf,
	'P','S','H','U',0x89,0x36,0xcf,0xcf,0xcf,
	'R','T','S',	0x81,0x39,0,0,0,
	'T','F','R',	0x8a,0x1f,0xcf,0xcf,0xcf,
	'A','S','L','A',0x81,0x48,0,0,0,
	'A','S','L','B',0x81,0x58,0,0,0,
	'A','S','R','A',0x81,0x47,0,0,0,
	'A','S','R','B',0x81,0x57,0,0,0,
	'C','L','R','A',0x81,0x4f,0,0,0,
	'C','L','R','B',0x81,0x5f,0,0,0,
	'C','O','M','A',0x81,0x43,0,0,0,
	'C','O','M','B',0x81,0x53,0,0,0,
	'D','A','A',	0x81,0x19,0,0,0,
	'L','S','L','A',0x81,0x48,0,0,0,
	'L','S','L','B',0x81,0x58,0,0,0,
	'L','S','R','A',0x81,0x44,0,0,0,
	'L','S','R','B',0x81,0x54,0,0,0,
	'M','U','L',	0x81,0x3d,0,0,0,
	'N','E','G','A',0x81,0x40,0,0,0,
	'N','E','G','B',0x81,0x50,0,0,0,
	'R','O','L','A',0x81,0x49,0,0,0,
	'R','O','L','B',0x81,0x59,0,0,0,
	'R','O','R','A',0x81,0x46,0,0,0,
	'R','O','R','B',0x81,0x56,0,0,0,
	'R','T','I',	0x81,0x3b,0,0,0,
	'S','E','X',	0x81,0x1d,0,0,0,
	'S','W','I',	0x81,0x3f,0,0,0,
	'S','W','I','2',0x81,0x10,0x3f,0,0,
	'S','W','I','3',0x81,0x11,0x3f,0,0,
	'S','Y','N','C',0x81,0x13,0,0,0,
	'T','S','T','A',0x81,0x4d,0,0,0,
	'T','S','T','B',0x81,0x5d,0,0,0,
	'A','D','C','A',0x82,0x89,0x99,0xa9,0xb9,
	'A','D','C','B',0x82,0xc9,0xd9,0xe9,0xf9,
	'A','D','D','A',0x82,0x8b,0x9b,0xab,0xbb,
	'A','D','D','B',0x82,0xcb,0xdb,0xeb,0xfb,
	'A','D','D','D',0x83,0xc3,0xd3,0xe3,0xf3,
	'A','N','D','A',0x82,0x84,0x94,0xa4,0xb4,
	'A','N','D','B',0x82,0xc4,0xd4,0xe4,0xf4,
	'A','N','D','C','C',0x82,0x1c,0xcf,0xcf,0xcf,
	'A','S','L',	0x82,0xcf,0x08,0x68,0x78,
	'A','S','R',	0x82,0xcf,0x07,0x67,0x77,
	'B','I','T','A',0x82,0x85,0x95,0xa5,0xb5,
	'B','I','T','B',0x82,0xc5,0xd5,0xe5,0xf5,
	'C','L','R',	0x82,0xcf,0x0f,0x6f,0x7f,
	'C','M','P','B',0x82,0xc1,0xd1,0xe1,0xf1,
	'C','M','P','X',0x83,0x8c,0x9c,0xac,0xbc,
	'C','O','M',	0x82,0xcf,0x03,0x63,0x73,
	'C','W','A','I',0x82,0x3c,0xcf,0xcf,0xcf,
	'D','E','C',	0x82,0xcf,0x0a,0x6a,0x7a,
	'I','N','C',	0x82,0xcf,0x0c,0x6c,0x7c,
	'E','O','R','A',0x82,0x88,0x98,0xa8,0xb8,
	'E','O','R','B',0x82,0xc8,0xd8,0xe8,0xf8,
	'J','M','P',	0x83,0xcf,0x0e,0x6e,0x7e,
	'J','S','R',	0x83,0xcf,0x9d,0xad,0xbd,
	'L','D','U',	0x83,0xce,0xde,0xee,0xfe,
	'L','E','A','S',0x82,0xcf,0xcf,0x32,0xcf,
	'L','E','A','U',0x82,0xcf,0xcf,0x33,0xcf,
	'L','E','A','X',0x82,0xcf,0xcf,0x30,0xcf,
	'L','E','A','Y',0x82,0xcf,0xcf,0x31,0xcf,
	'L','S','L',	0x82,0xcf,0x08,0x68,0x78,
	'L','S','R',	0x82,0xcf,0x04,0x64,0x74,
	'N','E','G',	0x82,0xcf,0x00,0x60,0x70,
	'O','R','A',	0x82,0x8a,0x9a,0xaa,0xba,
	'O','R','B',	0x82,0xca,0xda,0xea,0xfa,
	'O','R','C','C',0x82,0x1a,0xcf,0xcf,0xcf,
	'R','O','L',	0x82,0xcf,0x09,0x69,0x79,
	'R','O','R',	0x82,0xcf,0x06,0x66,0x76,
	'S','B','C','A',0x82,0x82,0x92,0xa2,0xb2,
	'S','B','C','B',0x82,0xc2,0xd2,0xe2,0xf2,
	'S','T','U',	0x83,0xcf,0xdf,0xef,0xff,
	'S','U','B','A',0x82,0x80,0x90,0xa0,0xb0,
	'S','U','B','B',0x82,0xc0,0xd0,0xe0,0xf0,
	'S','U','B','D',0x83,0x83,0x93,0xa3,0xb3,
	'T','S','T',	0x82,0xcf,0x0d,0x6d,0x7d,
	'C','M','P','D',0x84,0x83,0x93,0xa3,0xb3,
	'C','M','P','Y',0x84,0x8c,0x9c,0xac,0xbc,
	'C','M','P','S',0x85,0x8c,0x9c,0xac,0xbc,
	'C','M','P','U',0x85,0x83,0x93,0xa3,0xb3,
	'L','D','S',	0x84,0xce,0xde,0xee,0xfe,
	'S','T','S',	0x84,0xcf,0xdf,0xef,0xff,
	'S','T','Y',	0x84,0xcf,0x9f,0xaf,0xbf,
	'B','R','N',	0x86,0x21,0xcf,0xcf,0xcf,
	'B','H','I',	0x86,0x22,0xfc,0xcf,0xcf,
	'B','L','S',	0x86,0x23,0xcf,0xcf,0xcf,
	'B','H','S',	0x86,0x24,0xcf,0xcf,0xcf,
	'B','C','C',	0x86,0x24,0xcf,0xcf,0xcf,
	'B','L','O',	0x86,0x25,0xcf,0xcf,0xcf,
	'B','C','S',	0x86,0x25,0xcf,0xcf,0xcf,
	'B','V','C',	0x86,0x28,0xcf,0xcf,0xcf,
	'B','V','S',	0x86,0x29,0xcf,0xcf,0xcf,
	'B','P','L',	0x86,0x2a,0xcf,0xcf,0xcf,
	'B','M','I',	0x86,0x2b,0xcf,0xcf,0xcf,
	'B','G','E',	0x86,0x2c,0xcf,0xcf,0xcf,
	'B','L','T',	0x86,0x2d,0xcf,0xcf,0xcf,
	'B','G','T',	0x86,0x2e,0xcf,0xcf,0xcf,
	'B','L','E',	0x86,0x2f,0xcf,0xcf,0xcf,
	'B','S','R',	0x86,0x8d,0xcf,0xcf,0xcf,
	'L','B','R','N',0x87,0x10,0x21,0xcf,0xcf,
	'L','B','H','I',0x87,0x10,0x22,0xcf,0xcf,
	'L','B','L','S',0x87,0x10,0x23,0xcf,0xcf,
	'L','B','H','S',0x87,0x10,0x24,0xcf,0xcf,
	'L','B','C','C',0x87,0x10,0x24,0xcf,0xcf,
	'L','B','L','O',0x87,0x10,0x25,0xcf,0xcf,
	'L','B','C','S',0x87,0x10,0x25,0xcf,0xcf,
	'L','B','N','E',0x87,0x10,0x26,0xcf,0xcf,
	'L','B','E','Q',0x87,0x10,0x27,0xcf,0xcf,
	'L','B','V','C',0x87,0x10,0x28,0xcf,0xcf,
	'L','B','V','S',0x87,0x10,0x29,0xcf,0xcf,
	'L','B','P','L',0x87,0x10,0x2a,0xcf,0xcf,
	'L','B','M','I',0x87,0x10,0x2b,0xcf,0xcf,
	'L','B','G','E',0x87,0x10,0x2c,0xcf,0xcf,
	'L','B','L','T',0x87,0x10,0x2d,0xcf,0xcf,
	'L','B','G','T',0x87,0x10,0x2e,0xcf,0xcf,
	'L','B','L','E',0x87,0x10,0x2f,0xcf,0xcf,
	'E','X','G',	0x8a,0x1e,0xcf,0xcf,0xcf,
	'A','B','X',	0x81,0x3a,0,0,0,
	'N','O','P',	0x81,0x12,0,0,0,
/* Directives */
	'E','Q','U',	228, 0, 0, 0, 0,
	'O','R','G',	229, 0, 0, 0, 0,
	'F','C','B',	230, 0, 0, 0, 0,
	'F','D','B',	231,255,0, 0, 0,
	'R','D','B',	231, 0, 0, 0, 0,
	'F','C','C',	232, 0, 0, 0, 0,
	'F','C','C','H',232,128,0, 0, 0,
	'F','C','C','Z',232,0,255, 0, 0,
	'R','M','B',	233, 0, 0, 0, 0,
	'D','B',		230, 0, 0, 0, 0,
	'D','W',		231,255,0, 0, 0,
	'D','R','W',	231, 0, 0, 0, 0,
	'S','T','R',	232, 0, 0, 0, 0,
	'S','T','R','H',232,128,0, 0, 0,
	'S','T','R','Z',232,0,255, 0, 0,
	'D','S',		233, 0, 0, 0, 0,
	'S','E','T','D','P',234, 0, 0, 0, 0,
	'E','N','D',	235, 0, 0, 0, 0,
	'P','A','G','E',248, 0, 0, 0, 0,
	'T','I','T','L','E',249,0, 0, 0, 0,
	'S','P','A','C','E',250,0, 0, 0, 0,
	'L','I','S','T',	251,0, 0, 0, 0,
	'N','O','L','I','S','T', 252, 0, 0, 0, 0,
	'L','I','N','E',253,0, 0, 0, 0,
	247,0,0,0,0,	/* null instruction for blank lines */
	0 } ;

/* error messages */
static char const *const etext[] = { "?",
	"Unknown instruction",
	"Out of range",
	"Invalid addressing mode",
	"Invalid register specification",
	"Undefined symbol",
	"Invalid expression syntax",
	"Invalid argument format",
	"Improperly delimited string" } ;

/* push pull register translation */
static unsigned char const xlat[] = { 0x06, 0x10, 0x20, 0x40, 0x40, 0x80, 0, 0, 0x02, 0x04,
				0x01, 0x08 } ;

/* Symbol table & free pointer */
static unsigned char symtab[SYMB_POOL], *symtop;

/* misc. global variables */
static char buffer[LINE_SIZE+1], iinfo[4], label[SYMB_SIZE+1], operand[200], title[50];
static unsigned char post, itype, otype; /* opcode type codes exceed 127 - must be unsigned */
static int optr;                         /* array index into operand[200] - must be int */
static unsigned char instruction[80], outrec[35];

static char optf=3, symf=0, intel=0, fulf=0, quietf=0, nlist=0, casf=0;

static unsigned value, curdp, length, addr, ocnt, ecnt, emsg, line, lcount,
	pcount, pagel=59, pagew=80;

static FILE *asm_fp, *hex_fp, *lst_fp;

/*
 * Define a symbol in the symbol table
 */
static char *create_symbol(char *symbol, int value)
{
	unsigned char *ptr, *ptr1;

	ptr = ptr1 = symtop;
	while(*symbol)
		*++ptr = *symbol++;
	*ptr1 = ptr - ptr1;
	*++ptr = value >> 8;
	*++ptr = value;
	if(ptr < (symtab+sizeof(symtab))) {
		symtop = ptr + 1;
		return (char *)ptr1; }
	return 0;
}

/*
 * Lookup a symbol in the symbol table
 */
static char *lookup_symbol(char *symbol)
{
	int l;
	unsigned char *ptr, *ptr1, *ptr2;

	ptr = symtab;
again:
	if((ptr2 = ptr) >= symtop) {
		value = 0x8888;
		return 0; }
	ptr1 = (unsigned char *)symbol;
	l = *ptr++ & SYMMASK;
	while(l--) {
		if(*ptr1++ != *ptr++) {
			ptr += l + 2;
			goto again; } }
	if(*ptr1) {
		ptr += 2;
		goto again; }
	value = *ptr++ << 8;
	value |= *ptr;
	return (char *)ptr2;
}

/*
 * Set the value for a symbol in the symbol table
 */
static void set_symbol(char *ptr, unsigned int value)
{
	ptr += (*ptr & SYMMASK);
	*++ptr = value >> 8;
	*++ptr = value;
}

/*
 * Display the symbol table (sorted)
 */
static void dump_symbols(void)
{
	unsigned char *ptr, *hptr;
	unsigned int l, h, w;

	fprintf(lst_fp, "SYMBOL TABLE:\n\n");
	w = 0;
	for(;;) {
		for(hptr = symtab; hptr < symtop; hptr += (*hptr & SYMMASK) + 3)
			if(!(*hptr & SYMSORT))
				goto found;
		putc('\n', lst_fp);
		return;
found:	for(ptr = (*hptr & SYMMASK) + hptr + 3; ptr < symtop; ptr += (*ptr & SYMMASK) + 3) {
			if(*ptr & SYMSORT)
				continue;
			if(compsym(ptr, hptr) < 0)
				hptr = ptr; }
		*(ptr = hptr) |= SYMSORT;
		h = l = *ptr++ & SYMMASK;
		if((w + l + 7) >= pagew) {			/* Overrun page length */
			putc('\n', lst_fp);
			w = 0; }
		if(w) {								/* Not a start of line - separate */
			fprintf(lst_fp, "   ");
			w += 3; }
		while(l--)
			putc(*ptr++, lst_fp);
		w += (l = (h > 8) ? 24 : 8) + 5;	/* Calculate extended length */
		while(h++ < l)
			putc(' ', lst_fp);
		l = *ptr++ << 8;
		l |= *ptr++;
		fprintf(lst_fp, "-%04x", l); }
}

/*
 * Compare two symbol table entries
 */
static int compsym(unsigned char *sym1, unsigned char *sym2)
{
	int l, l1, l2;
	l1 = *sym1++ & SYMMASK;
	l2 = *sym2++ & SYMMASK;
	l = (l1 < l2) ? l1 : l2;
	do {
		if(*sym1 > *sym2)
			return 1;
		if(*sym1++ < *sym2++)
			return -1; }
	while(--l);
	if(l1 > l2)
		return 1;
	if(l1 < l2)
		return -1;
	return 0;
}

/************************************/
/* get a character from the operand */
/************************************/
static char getchr(void)
{
	char chr;

	if((chr=operand[optr])) ++optr;
	return chupper(chr);
}

/*
 * Convert character to upper case if NOT case sensitive
 */
static char chupper(char c)
{
	return casf ? c : ((c >= 'a') && (c <= 'z')) ? c - ('a'-'A') : c;
}

/*
 * Open a filename with the appriopriate extension &
 * report an error if not possible
 */
static FILE *open_file(char *filename, char *extension, char *options)
{
	char buffer[100], *ptr, *dot;
	FILE *fp;

	dot = 0;

	for(ptr = buffer; (*ptr = *filename); ++ptr) {	/* Copy filename */
		if(*filename == '.')
			dot = filename;
		else if(*filename == '\\')
			dot = 0;
		++filename; }

	if(!dot) {									/* No extension supplied */
		*ptr++ = '.';
		do
			*ptr++ = *extension;
		while(*extension++); }
	else
		*dot = 0;

	if(!(fp = fopen(buffer, options))) {
		fprintf(stderr,"Unable to access: '%s'\n", buffer);
		exit(1); }

	return fp;
}

/*
 * Main program
 */
int main(int argc,char *argv[])
{	unsigned vbyt, temp, daddr;
	int stemp;
	char pflg, cflg, chr, *ptr, *lfile, *cfile;

	if(argc < 2)
		die("\nUse: asm09 <filename> [-cfiqst c=file l=file o=n p=length w=width]\n\n?COPY.TXT 1983-2005 Dave Dunfield\n**See COPY.TXT**.\n");

	pflg = 0;

/* parse for command line options. */
	lfile = cfile = argv[1];
	for(int i=2; i < argc; ++i) {
		if(*(ptr = argv[i]) == '-') {		/* Enable switch */
			while(*++ptr) switch(toupper(*ptr)) {
				case 'C' : casf = -1;		break;
				case 'F' : fulf = -1;		break;
				case 'I' : intel = -1;		break;
				case 'Q' : quietf = -1;		break;
				case 'S' : symf = -1;		break;
				case 'T' : lfile = 0;		break;
				default: goto badopt; }
			continue; }
		if(*++ptr == '=') switch(toupper(*(ptr++ - 1))) {
			case 'L' : lfile = ptr;			continue;
			case 'C' : cfile = ptr;			continue;
			case 'O' : optf=atoi(ptr);		continue;
			case 'P' : pagel=atoi(ptr)-1;	continue;
			case 'W' : pagew=atoi(ptr);		continue; }
	badopt:
		fprintf(stderr,"Invalid option: %s\n", argv[i]);
		exit(1); }

	asm_fp = open_file(argv[1], "ASM", "r");
	hex_fp = open_file(cfile, "HEX", "w");
	lst_fp = (lfile) ? open_file(lfile, "LST", "w") : stdout;
	strncpy(title,argv[1],sizeof(title)-1); // XXX

/* first pass - build symbol table */
	symtop = symtab;
	ocnt = ecnt = 0;
	do {
		addr = cflg = 0; line = 1; curdp = -1;
		if(!quietf){	if(pflg) fprintf(stderr,"Opt... ");
					else fprintf(stderr,"First pass... ");}
		while(readline()) {
			if(!optr) {
				locins(instruction);
				if(label[0]) {				/* label exists */
					if(pflg) {				/* optomization pass */
						ptr = lookup_symbol(label);
						if(itype==100) {	/* EQU statement */
							optr = 0; vbyt = value;
							if((temp=eval()) != vbyt) {
								if(pflg >= optf) optfail();
								set_symbol(ptr, temp);
								cflg=1; } }
						else if(value != addr) {
							if(pflg >= optf) optfail();
							set_symbol(ptr, addr);
							cflg=1; } }
					else {							/* original pass-1 */
						if(lookup_symbol(label)) {	/* duplicate label */
							fprintf(lst_fp,"** Line %u - Duplicate symbol: '%s'\n",line,label);
							++ecnt; }
						else {
							cflg=1;			/* indicate symbol table change */
							if(!(ptr = create_symbol(label, addr))) {
								xabort("symbol");
								fprintf(lst_fp,"** Line %u - Symbol table overflow\n", line);
								++ecnt;
								break; } } } }
				emsg = length = optr = 0;
				switch(itype) {
					case 1 :			/* inherent addressing */
						length=1+(iinfo[1]!=0);
						break;
					case 2 :			/* eight bit immediate data */
						if(memop()) ++length;
						break;
					case 3 :			/* 16 bit immedaite data */
						memop();
						++length;
						break;
					case 4 :			/* 16 bit imm with $10 prefix. */
						goto typ4;
					case 5 :			/* 16 bit imm with $11 prefix. */
typ4:					memop();
twobyt:					length += 2;
						break;
					case 6 :			/* short branches. */
						goto twobyt;
					case 7 :			/* long conditionals. */
						length += 4;
						break;
					case 8 :			/* lbra and lbsr */
						length += 3;
						break;
					case 9 :			/* push and pull */
						goto twobyt;
					case 10 :			/* transfer and exchange */
						goto twobyt;
					case 100 :			/* EQU directive */
						if(!pflg) {
							set_symbol(ptr, eval());
						err1st:
							if(emsg && !optf) {
								fprintf(lst_fp,"** Line %u - %s\n",line,etext[emsg]);
								++ecnt; } }
						break;
					case 101 :			/* ORG directive */
						addr=eval();
						goto err1st;
					case 102 :			/* FCB statement */
						length=1;
						while(nxtelem()) ++length;
						break;
					case 103 :			/* FDB statement */
						length=2;
						while(nxtelem()) length += 2;
						break;
					case 104 :			/* FCC statements */
						chr=operand[0];
						for(int i=1;(operand[i])&&(chr!=operand[i]); ++i) ++length;
						if(iinfo[1]) ++length;
						break;
					case 105 :			/* RMB statement */
						addr += eval();
						goto err1st;
					case 106 :			/* SETDP statement */
						curdp = eval();
						break;
					case 107 :			/* END statement */
						goto end1;
					case 125 :			/* LINE directive */
						line = eval(); } }
			else
				length=0;
			addr += length;
			++line; /* += (itype<120); */ }
end1:	++pflg;
		rewind(asm_fp); }
	while((optf) && (cflg) && (!ecnt));

/* second pass - generate object code */

	if(!quietf) fprintf(stderr,"Second pass... ");
	addr = emsg = 0; line = pcount = 1; curdp = -1; lcount = 9999;

	while(readline()) {
		daddr = addr;
		if(!optr) {
			if(!locins(instruction)) reperr(1);		/* unknown opcode */
			length = temp = vbyt = optr = 0;
			switch(itype) {
				case 1 :			/* inherent addressing */
					instruction[length++]=iinfo[0];
					if(iinfo[1]) instruction[length++] = iinfo[1];
				break;
				case 2 :			/* 8 bit memory reference. */
					if((otype=memop())) ++length;
pnimm:				instruction[temp++]=iinfo[otype];
					vbyt=length-1;
					if(otype==2) {
						instruction[temp++]=post;
						--vbyt; }
					break;
				case 3 :			/* 16 bit memory reference */
					otype=memop();
					++length;
					goto pnimm;
				case 4 :			/* same as 3 but $10 prefix */
					instruction[temp++] = 0x10;
					goto p2t5;
				case 5 :			/* same as 3 but $11 prefix. */
					instruction[temp++] = 0x11;
p2t5:				otype=memop();
					length += 2;
					instruction[temp++]=iinfo[otype];
					vbyt=length-2;
					if(otype==2) {
						instruction[temp++]=post;
						--vbyt; }
					break;
				case 6 :			/* short branches */
					instruction[temp++] = iinfo[0];
					stemp = value = (eval() - addr) - 2;
					vbyt=1;
					if((stemp>127)||(stemp < -128)) reperr(2); /* out or range */
					length = 2;
					break;
				case 7 :			/* long conditionals */
					instruction[temp++] = iinfo[0];
					instruction[temp++] = iinfo[1];
					value = (eval() - addr) - 4;
					vbyt = 2;
					length = 4;
					break;
				case 8 :			/* lbsr and lbra */
					instruction[temp++] = iinfo[0];
					value = (eval() - addr) - 3;
					vbyt = 2;
					length = 3;
					break ;
				case 9 :		/* pshs and pul */
					instruction[temp++] = iinfo[0];
					post = 0;
					do {
					post = post | xlat[getreg()]; }
					while(getchr() == ',');
					instruction[temp++]=post;
					length = 2;
					break ;
				case 10 :		/* transfer and exchange */
					instruction[temp++] = iinfo[0];
					int i = getreg();
					if(getchr() != ',') reperr(7);
					instruction[temp++] = (i << 4) | getreg();
					length = 2;
					break;
				case 100 :			/* equate statement */
					daddr=eval();	/* generate errors if any */
					break;
				case 101 :			/* ORG statement */
					if(ocnt) wrrec();
					daddr=addr=eval();
					break;
				case 102 :			/* FCB statement */
					do {
						instruction[length++] = eval(); }
					while(operand[optr-1] == ',');
					break;
				case 103 :			/* FDB statement */
					do {
						temp = eval();
						if(iinfo[0]) {
							instruction[length++] = temp >> 8;
							instruction[length++] = temp; }
						else {
							instruction[length++] = temp;
							instruction[length++] = temp >> 8; } }
					while(operand[optr-1] == ',');
					break;
				case 104 :			/* FCC statements */
					chr=operand[0];
					for(i=1;((operand[i])&&(chr!=operand[i])); ++i)
						instruction[length++] = operand[i];
					if(!operand[i]) reperr(8);
					instruction[length-1] = instruction[length-1] | iinfo[0];
					if(iinfo[1]) instruction[length++]=0;
					break;
				case 105 :			/* RMB statement */
					if(ocnt) wrrec();
					addr += eval();
					break;
				case 106 :			/* SETDP statement */
					curdp = eval();
					break;
				case 107 :			/* END statement */
					goto end2;
				case 120 :			/* PAGE statement */
					lcount=9999;
					break;
				case 121 :			/* TITLE directive */
					strncpy(title, operand, sizeof(title)-1); // XXX
					break;
				case 122 :			/* SPACE directive */
					fprintf(lst_fp,"\n");
					++lcount;
					break;
				case 123 :			/* LIST directive */
					if(nlist) --nlist;
					break;
				case 124 :			/* NOLIST directive */
					++nlist;
					break;
				case 125 :			/* LINE directive */
					line = eval(); }
			if((instruction[0]==0xffcf)&&(itype<100)) reperr(3); /* invalid addressing mode */ // XXX
			if(vbyt==2) instruction[temp++]=value >> 8;
			if(vbyt) instruction[temp++]=value; }
			else length = itype = 0;
/* generate listing */
	if(((itype<120) && fulf && !nlist) || emsg) {
		if(++lcount >= pagel)
			write_title();
		fprintf(lst_fp,"%04x ",daddr);
			for(unsigned int i=0; i < 6; ++i) {
				if(i < length)
					fprintf(lst_fp," %02x",instruction[i]);
				else
					fprintf(lst_fp,"   "); }
			fprintf(lst_fp," %c%5u  %s",(length <= 6) ? ' ' : '+', line,buffer); }
	if(emsg) {
		fprintf(lst_fp,"  ** ERROR ** - %u - %s\n",emsg,etext[emsg]);
		++ecnt; ++lcount;
		emsg=0; }
	++line; /* += (itype<120); */
/* write code to output record */
	for(unsigned int i=0; i < length; ++i) {
		if(!ocnt) {			/* first byte of record */
			outrec[ocnt++]=addr>>8;
			outrec[ocnt++]=addr; }
		++addr;
		outrec[ocnt++]=instruction[i];
		if(ocnt > 33) wrrec(); } }	/* record is full, write it */

/* end of assemble, output final record */
end2:
	if(ocnt) wrrec();
	if(intel) fprintf(hex_fp,":00000001FF\n");
	else fprintf(hex_fp,"S9030000FC\n");

	if(ecnt) fprintf(lst_fp,"\n %u error(s) occurred in this assembly.\n",ecnt);
	if(!quietf) fprintf(stderr,"%u error(s).\n",ecnt);

/* display the symbol table */
	if(symf) {
		write_title();
		dump_symbols(); }

	fclose(asm_fp);
	fclose(hex_fp);
	fclose(lst_fp);

	return ecnt ? -2 : 0;
}

/***********************************************/
/* lookup instruction in the instruction table */
/***********************************************/
static bool locins(unsigned char inst[])
{
	/*---------------------------------------------------------------
	; XXX - Changing *ptr to "unsigned char" make the assembler fail
	;----------------------------------------------------------------*/
	char *ptr, *ptr1;

	ptr = (char *)opcodes;
	do {
		ptr1 = (char *)inst;
		while(*ptr == *ptr1) {
			++ptr;
			++ptr1; }
		if((*ptr < 0) && !*ptr1) {
			itype = *ptr++ & 127;
			for(int i=0; i < 4; ++i)
				iinfo[i] = *ptr++;
			return true; }
		while(*ptr > 0)
			++ptr; }
	while(*(ptr += 5));

	return(itype = 0);
}

/*************************************************/
/* read a line from input file, and break it up. */
/*************************************************/
static bool readline(void)
{
	size_t i;
	char *ptr;

	if(!fgets(ptr = buffer, LINE_SIZE, asm_fp))
		return false;

	i = 0;
	while(issymbol(*ptr)) {
		label[i] = chupper(*ptr++);
		if(i < SYMB_SIZE)
			++i; }
	label[i]=0;
	if(*ptr == ':')
		++ptr;
	while(isspace(*ptr))
		++ptr;
	if(((*ptr != '*') && (*ptr != ';')) || i) {
		i = 0;
		while(*ptr && !isspace(*ptr))
			instruction[i++] = toupper(*ptr++);
		instruction[i]=0;
		while(isspace(*ptr))
			++ptr;
		optr = i = 0;
		while(*ptr && (i < (sizeof(operand)-1)))
			operand[i++] = *ptr++;
		operand[i] = 0;
		return true; }

	optr = 1;
	return true;
}

/***************************************/
/* test for valid terminator character */
/***************************************/
static bool isterm(char chr)
{
	switch(chr) {
		case 0 :
		case '\n':  /* fgets includes newline; treat as terminator */
		case '\r':  /* guard against any stray CR */
		case ';' :  /* inline comment */
		case ' ' :
		case '\t':
		case ',' :
		case ']' :
		case ')' :
			return true; }
	return false;
}

/*************************************/
/* Test for a valid symbol character */
/*************************************/
static bool issymbol(char c)
{
	switch(c) {			/* Special allowed characters */
		case '_' :
		case '?' :
		case '!' :
		case '.' :
			return true; }

	return isalnum(c);
}

/*********************************************/
/* evaluate indexed memory reference operand */
/*********************************************/
static int evlind(unsigned char indir)
{
	unsigned temp;
	int stemp;
	char chr;
	temp=(operand[optr+1] == ',');
	chr=chupper(operand[optr]);
	if(temp&&((chr=='A')||(chr=='B')||(chr=='D'))) {	/* accumulator offset */
		if(chr=='A') post=0x86;
		else post = (chr=='B') ? 0x85 : 0x8B;
		optr += 2 ;
		if((temp=addreg())) return temp;
		length=1;
		return 2; }
	else if(chr==',') {				/* no offset */
		post=0x84;
		++optr;
		if((temp=addreg())) return temp;
		length=1;
		return 2; }
	else {
		value=eval();
		if(operand[optr-1] != ',') {	/* not a constant offset */
			if(indir&1) {				/* extended indirect */
				post=0x8f;
				length=3;
				return 2; }
			else {					/* extended or dp */
				if((optf)&&((value >> 8)==curdp)) {	/* use dp */
					length=1;
					return 1; }
				else {
					length=2;
					return 3; } } }
		else {						/* constant offset from register */
			stemp=value;
			if(indir & 0x02) goto fc8;
			if(indir & 0x04) goto fc16;
			if((!(indir&1))&&(optf)&&(stemp<16)&&(stemp>= -16)) {	/* 5 bit */
				post= value & 0x1f;
				if((temp=addreg())) return temp;
				length=1;
				return 2; }
			else if ((optf)&&(stemp<128)&&(stemp>= -128)) {	/* 8 bit */
			fc8:
				post=0x88;
				if((temp=addreg())) return temp;
				length=2;
				return 2; }
			else {						/* 16 bit offset */
			fc16:
				post=0x89;
				if((temp=addreg())) return temp;
				length=3;
				return 2; } } }
}

/**************************/
/* process memory operand */
/**************************/
static int memop(void)
{
	unsigned temp;
	char chr;
	optr=0;
	if((chr=chupper(operand[optr++])) == '#') {	/* immediate addressing */
		value=eval();
		length=2;
		return 0; }
	else if(chr=='>') {							/* extended addressing */
		value=eval();
		if(operand[optr-1] == ',') {
			optr = 1;
			return evlind(0x04); }
		length=2;
		return 3; }
	else if(chr=='<') {							/* direct page addressing */
		value=eval();
		if(operand[optr-1] == ',') {
			optr = 1;
			return evlind(0x02); }
		length=1;
		return 1; }
	else if(chr=='[') {							/* indirect addressing */
		switch(operand[optr]) {
		case '>' : ++optr; temp = evlind(0x05);	break;
		case '<' : ++optr; temp = evlind(0x03);	break;
		default: temp = evlind(0x01); }
		post = post | 0x10;
		return temp; }
	else {										/* indexed addressing */
		--optr;
		return evlind(0); }
}

/***************************************/
/* insert register value into postbyte */
/***************************************/
static int addreg(void)
{	char chr;
	int stemp;
	if((chr=getchr()) == '-') {			/* auto decrement */
		if((chr=getchr()) == '-') {
			post = 0x83;
			chr=getchr(); }
		else post = 0x82; }
	if(chr!='P') {
		if(chr=='Y') post = post | 32;
		else if(chr=='U') post = post | 64;
		else if(chr=='S') post = post | 96;
		else if(chr!='X') reperr(4);		/* invalid register */
		if(getchr() == '+')					/* auto increment */
			post = (post & 0x60) | ( (getchr() == '+') ? 0x81 : 0x80 );
		return(0); }
	else {									/* pc relative offsets */
		value = (value - addr) - 3;
		if((itype==4)||(itype==5)) value=value-1;
		stemp = value;
		if((optf)&&(stemp < 128)&&(stemp >= -128)) {	/* 8 bit offset */
			post=0x8c;
			length=2;
			return(2); }
		else {										/* 16 bit offset */
			--value;
			post=0x8d;
			length = 3;
			return(2); } }
}
	
/*
 * Evaluate an expression.
 */
static int eval(void)
{	char c;
	unsigned int result;

	result=getval();
	while(!isterm(c = getchr())) switch(c) {
		case '+' : result += getval();	break;
		case '-' : result -= getval();	break;
		case '*' : result *= getval();	break;
		case '/' : result /= getval();	break;
		case '\\': result %= getval();	break;
		case '&' : result &= getval();	break;
		case '|' : result |= getval();	break;
		case '^' : result ^= getval();	break;
		case '<' : result <<= getval();	break;
		case '>' : result >>= getval();	break;
		default: reperr(6); }

	return result;
}

/*
 * Get a single value (number or symbol)
 */
static int getval(void)
{
	int i, b, val;
	char chr, array[25], *ptr;

	switch(chr = operand[optr++]) {
		case '-' :				/* Negated value */
			return 0 - getval();
		case '~' :				/* Complemented value */
			return ~getval();
		case '=' :				/* Swap high and low bytes */
			i=getval();
			return (i<<8)+(i>>8);
		case '(' :				/* Nested expression */
			val = eval();
			if(operand[optr-1] != ')')
				reperr(6);
			return val; }
	--optr;

	val=b=i=0;
	switch(chr) {
		case '$' :	b = 16;	goto getnum;
		case '@' :	b = 8;	goto getnum;
		case '%' :	b = 2;	goto getnum; }
	if(isdigit(chr)) {
		array[i++] = chr;
	getnum:
		while(isalnum(chr = toupper(operand[++optr])))
			array[i++] = (chr < 'A') ? chr : chr - 7;
		if((b == 16) && !i)
			val = addr;
		else {
			if(!b) {
				b = 10;
				switch(array[i-1]) {
					case 'H'-7 :	b = 16;	goto deci;
					case 'T'-7 :
					case 'D'-7 :	b = 10;	goto deci;
					case 'Q'-7 :
					case 'O'-7 :	b = 8;	goto deci;
					case 'B'-7 :	b = 2;
					deci:	--i;	} }
			if(!i)
				reperr(6);
			for(ptr = array; i; --i) {
				if((chr = *ptr++ - '0') >= b)
					reperr(6);
				val = (val * b) + chr; } } }
	else if(chr=='\'') {				/* Single quote. */
		++optr;
		while(((chr=operand[optr++]) != '\'')&& chr)
			val=(val << 8)+chr;
		if(!chr) reperr(8); }
	else if(chr=='*') {					/* Program counter */
		++optr;
		val=addr; }
	else {								/* Must be a label */
		i = 0;
		while(issymbol(chr = chupper(operand[optr]))) {
			++optr;
			label[i]=chr;
			if(i < SYMB_SIZE)
				++i; }
		label[i]=0;

		if(!lookup_symbol(label)) reperr(5);
		val=value; }

	return val;
}

/* report an error */
static void reperr(int errn)
{	if(!emsg) emsg=errn;
}

/*************************************************/
/* decode a register value from the operand line */
/*************************************************/
static int getreg(void) {
	char chr;

	if((chr=getchr()) == 'C') {
		if(getchr() == 'C') return(10);
		else reperr(4); }
	else if(chr=='D') {
		if(chupper(operand[optr]) == 'P') {
			++optr;
			return(11); }
		else return(0); }
	else if(chr=='P') {
		if(getchr() == 'C') return(5);
		else reperr(4); }
	else if(chr=='A') return(8);
	else if(chr=='B') return(9);
	else if(chr=='X') return(1);
	else if(chr=='Y') return(2);
	else if(chr=='U') return(3);
	else if(chr=='S') return(4);
	else {
		reperr(4);
		return(0); }
	abort();
}

/**************************************/
/* find next element in argument list */
/**************************************/
static int nxtelem(void) {
	char chr;
	while((chr=operand[optr])&&(chr != ' ')&&(chr != 9)) {
		++optr;
		if(chr==39) {
			while((chr=operand[optr])&&(chr!=39)) ++optr;
			++optr; }
		if(chr==',') return(1); }
	return(0); }

/*******************************/
/* write record to output file */
/*******************************/
static void wrrec(void)
{	unsigned chk, chr;

	xhex(ocnt-2);
	if(intel) {					/* intel hex format */
		chk = outrec[0] + outrec[1] + ocnt - 2;
		fprintf(hex_fp,":%02x%02x%02x00", ocnt-2, outrec[0], outrec[1]);
		for(unsigned int i=2; i<ocnt; ++i) {
			fprintf(hex_fp,"%02x", chr = outrec[i]);
			chk += chr; }
		fprintf(hex_fp,"%02x\n", 255 & (0-chk)); }
	else {						/* motorola hex format */
		chk = ocnt + 1;
		fprintf(hex_fp,"S1%02x", ocnt + 1);
		for(unsigned int i=0; i<ocnt; ++i) {
			fprintf(hex_fp,"%02x", chr = outrec[i]);
			chk += chr; }
		fprintf(hex_fp,"%02x\n", 255 & ~chk); }
	ocnt = 0;
}

/*
 * Write out title
 */
static void write_title(void)
{
	char *ptr;

	if(pcount > 1)
		putc('\f',lst_fp);
	lcount = 1;
	fprintf(lst_fp,"DUNFIELD 6809 ASSEMBLER: ");
	ptr = title;
	for(unsigned int w = 35; w < pagew; ++w)
		putc(*ptr ? *ptr++ : ' ', lst_fp);
	fprintf(lst_fp,"PAGE: %u\n\n", pcount);
	++pcount;
}

/*
 * Too many optimization passes - report failure
 */
static void optfail(void)
{
	fprintf(lst_fp,"** Line %u - Unable to resolve: %s\n", line, label);
	++ecnt;
}
