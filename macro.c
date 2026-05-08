/*
 * Assembly source code MACRO pre-processor
 *
 * ?COPY.TXT 1983-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "portab.h"

#define	SPRINTF
#include "xasm.h"

#define	MAXFILES	50		/* maximum number of files */
#define LINSIZE		200		/* maximum line length */
#define NUMSMACS	1000	/* maximum number of SET macros */
#define	NUMIMACS	300		/* Maximum number of INST macros */
#define MACSPACE	50000	/* memory reserved for definitions */

/* To enable the following, set to "printf a;" */
#define	DEBUGIF(a)			/* Debug "if" statements */
#define DEBUG(a)			/* General debugging */

unsigned
	Da, Mo, Yr, Hr, Mi, Se,
	msnum,
	minum,
	macnum,
	ifnest,
	cur_line,
	desize;
FILE
	*fp,
	*ofp;		/* current file pointer */
unsigned char
	*input_ptr;			/* expression parser input (was unsigned*) */
unsigned char
	*macsub[NUMSMACS], *macins[NUMIMACS], *macend[NUMIMACS],
	buffer[MACSPACE],
	*deptr,
	linbuf[LINSIZE],
	instr[100],
	lable[100],
	stack[5],
	sptr,
	prefix,
	*freptr = buffer,
	need_line,
	adjline,
	cur_file[65];

//struct time todays_time;
//struct date todays_date;

static unsigned char *cmds[] = {
	"SET", "IFEQ", "IFNE", "IF", "ELSE", "ENDIF", "INCLUDE", "MACRO",
	"ENDMAC", "ABORT", "PREFIX", "ESET",
	0 };

static unsigned char *months[] = {
	"---", "January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December" };

/* Forward declarations — required before first call site on 64-bit */
unsigned char *process_line(unsigned char *, unsigned char *);
void read_file(unsigned char *);
unsigned char *skip_blank(unsigned char *);
unsigned char *skip_parm(unsigned, unsigned char *);
unsigned char *extract_parm(unsigned char *, unsigned char *);
void set_symbol(unsigned char *, unsigned char *);
void macro_sub(unsigned char *);
int parms_equal(unsigned char *);
void check_free(void);
void quit(unsigned char *);
unsigned get_value(void);
unsigned expression(void);
int isend(unsigned char);
int issymbol(unsigned char);

main(argc, argv)
	int argc;
	char *argv[];
{
	unsigned i, count, n, j;
	unsigned char c, *ptr, *files[MAXFILES], *nptr;
	unsigned char sym[100];
	count = 0;

	if(argc < 2)
		die("\nUse: macro [source files* symbol=value* -L -Pprefix] >outputfile\n\n?COPY.TXT 1983-2005 Dave Dunfield\n**See COPY.TXT**.\n");

	{ time_t _t = time(0); struct tm *_tm = localtime(&_t);
	  Da = _tm->tm_mday; Mo = _tm->tm_mon+1; Yr = _tm->tm_year+1900;
	  Hr = _tm->tm_hour; Mi = _tm->tm_min;  Se = _tm->tm_sec; }

	ofp = stdout;
	for(i=1; i < argc; ++i) {
		switch(*(ptr = argv[i])) {
		case '>' :
			if(!(ofp = fopen(ptr+1, "w"))) {
				fprintf(stderr,"Unable to write: '%s'\n", ptr+1);
				exit(-1); }
			continue;
		case '-' :
			switch(toupper(*(ptr+1))) {
			case 'L' : adjline = -1;		continue;
			case 'P' : prefix = *(ptr+2);	continue;
			} fprintf(stderr,"Invalid option: %s\n", ptr);
			exit(-1); }
		nptr = ptr;
		while(issymbol(c = *nptr))
			++nptr;
		if(c == '=') {		/* assign symbol value */
			n = nptr - ptr;
			if(n >= sizeof(sym))
				quit("Symbol name too long");
			for(j=0; j < n; ++j)
				sym[j] = toupper(ptr[j]);
			sym[n] = 0;
			set_symbol(sym, nptr+1); }
		else					/* process input file */
			files[count++] = argv[i]; }

	for(i=0; i < count; ++i)
		read_file(files[i]);

	return 0;
}

void read_file(fname)
	unsigned char *fname;
{
	unsigned savline;
	FILE *savfp;
	unsigned char savfile[65];

/* save file related info */
	savfp = fp;				/* save current file pointer*/
	strcpy(savfile, cur_file);
	savline	= cur_line;		/* save current line number */
	cur_line = 0;
	strcpy(cur_file, fname);

	if(!(fp = fopen(fname,"r"))) {
		fprintf(stderr,"Unable to access: '%s'\n", fname);
		exit(-1); }

	while(fgets(linbuf, LINSIZE, fp)) {
		++cur_line;
		process_line(linbuf,0); }

	fclose(fp);
	cur_line = savline;			/* restore current line number */
	strcpy(cur_file, savfile);	/* Restore current file name */
	fp = savfp;					/* restore current file pointer */
}

unsigned char *process_line(linebuf, mptr)
	unsigned char *linebuf, *mptr;
{
	unsigned i, temp, mnum;
	unsigned char chr, *linptr, *outptr, *macptr, *parptr, outbuf[LINSIZE];

	if((*linebuf == '*') || (*linebuf == ';')) { /* comment line is ignored */
		fputs(linebuf, ofp);
		putc('\n', ofp); }
	else {
		macptr = lable;
		for(linptr = linebuf; !isend(chr=*linptr); ++linptr)
			*macptr++ = toupper(chr);
		*macptr++ = 0;
		macro_sub(linebuf);						/* perform defined substution */
		linptr = linebuf;
		while(!isend(*linptr))					/* skip lable if any */
			++linptr;
		linptr = skip_blank(linptr);
		macptr = outptr = instr;
		while(!isend(*linptr))
			*macptr++ = toupper(*linptr++);
		*macptr = 0;
		linptr = skip_blank(linptr);			/* point to parameters */
		if(prefix) {
			if(*outptr != prefix)
				goto nodirect;
			++outptr; }
		for(i=0; cmds[i]; ++i) {
			if(!strcmp(outptr, cmds[i]))
				break; }
		switch(i) {
		case 11 :				/* ESET command */
			extract_parm(linptr, instr);
			input_ptr = skip_parm(1, linptr);
			sprintf(linptr, instr, expression());
		case 0 :				/* set command */
			set_symbol(lable, linptr);
			break;
		case 1 :				/* IFEQ command */
			++ifnest;
DEBUGIF(("IFEQ %u\n", ifnest))
			if(!parms_equal(linptr))
				goto skip_to_end;
			break;
		case 2 :				/* IFNE command */
			++ifnest;
DEBUGIF(("IFNE %u\n", ifnest))
			if(parms_equal(linptr))
				goto skip_to_end;
			break;
		case 3 :				/* IF command */
			++ifnest;
DEBUGIF(("IF %u\n", ifnest))
			input_ptr = linptr;
			if(!expression()) {
skip_to_end:	temp = ifnest;
DEBUGIF(("TEMP %u\n", temp))
				for(;;) {
					macptr = outptr = lable;
					if(mptr) {		/* conditional within macro */
						while(((chr = *mptr) == ' ') || (chr == 9))
							++mptr;
						if(!chr)
							quit("Unterminated IF/ELSE in MACRO");
						while(!isend(*mptr))
							*macptr++ = toupper(*mptr++);
						while(*mptr++); }
					else {
						if(!(linptr=fgets(outbuf,LINSIZE,fp)))
							quit("Unterminated IF/ELSE");
						++cur_line;
						linptr = skip_blank(linptr);
						while(!isend(chr=*linptr++))
							*macptr++ = toupper(chr); }
					*macptr = 0;
					if(prefix) {
						if(*outptr != prefix)
							continue;
						++outptr; }
					if(!strcmp(outptr, "IFEQ")) {
						++ifnest;
DEBUGIF(("IFEQ %u\n", ifnest))
						continue; }
					if(!strcmp(outptr, "IFNE")) {
						++ifnest;
DEBUGIF(("IFNE %u\n", ifnest))
						continue; }
					if(!strcmp(outptr, "IF")) {
						++ifnest;
DEBUGIF(("IF %u\n", ifnest))
						continue; }
					if(!strcmp(outptr, "ELSE")) {
DEBUGIF(("ELSE %u\n", ifnest))
						if(ifnest != temp)
							continue;
						break; }
					if(!strcmp(outptr, "ENDIF")) {
DEBUGIF(("ENDIF %u\n", ifnest))
						if(ifnest-- != temp) {
							continue; }
						break; } } }
DEBUGIF(("BREAK\n"))
			break;
		case 4 :				/* ELSE command */
DEBUGIF(("ELSE %u\n", ifnest))
			if(ifnest)
				goto skip_to_end;
			quit("ELSE without IF");
			break;
		case 5 :				/* ENDIF command */
DEBUGIF(("ENDIF %u\n", ifnest))
			if(!ifnest)
				quit("ENDIF without IF");
			--ifnest;
			break;
		case 6 :				/* INCLUDE command */
			extract_parm(linptr, instr);
			read_file(instr);
			break;
		case 7 :				/* MACRO command */
			macptr = lable;
			if(minum >= NUMIMACS)
				quit("Instrucion MACRO pool exausted");
			macins[minum] = freptr;
			while(!isend(chr=*macptr++))
				*freptr++ = toupper(chr);
			*freptr++ = 0;
			check_free();
			for(;;) {
				if(!(linptr = fgets(linbuf,LINSIZE,fp)))
					quit("Un-ended MACRO");
				++cur_line;
				linptr = skip_blank(linptr);
				macptr = lable;
				while(!isend(chr=*linptr++))
					*macptr++ = toupper(chr);
				*macptr = 0;
				if(prefix) {
					if((*lable == prefix) && !strcmp(lable+1, "ENDMAC"))
						break; }
				else if(!strcmp(lable, "ENDMAC"))
					break;
				linptr = linbuf;
				do
					*freptr++ = chr = *linptr++;
				while(chr);
				check_free(); }
			macend[minum++] = freptr;
			break;
		case 8 :				/* ENDMAC command */
			quit("ENDMAC without MACRO");
		case 9 :				/* ABORT command */
			extract_parm(linptr, instr);
			quit(instr);
		case 10 :				/* PREFIX command */
			prefix = *linptr;
			break;
		default:
		nodirect:
			for(i=0; i < minum; ++i) {
				if(!strcmp(instr, macptr=macins[i])) {	/* insert instruction */
					mnum = ++macnum;
					while(*macptr++);				/* advance to first line */
					do {
						outptr = outbuf;
						while(chr=*macptr++) {
							if(chr != '\\')
								*outptr++ = chr;
							else if((chr=*macptr++) == '\\')
								*outptr++ = chr;
							else if(chr == '0') {			/* put in label */
								parptr = linebuf;
								while(!isend(*parptr)) *outptr++ = *parptr++; }
							else if(isdigit(chr)) {		/* put in param. */
								parptr = skip_parm(chr-'1',linptr);
								outptr = extract_parm(parptr, outptr); }
							else if(chr == '$') {			/* macro number */
								temp = mnum;
								goto write_num; }
							else if(chr == '#') {			/* number of parameters */
								parptr = linptr;
								temp = (*parptr != 0);
								while((chr = *parptr++) && (chr!=9) && (chr!=' ')) {
									if(chr == ',') ++temp;
									else if(chr == '"')
										while('"'!=*parptr++); }
write_num:						sptr = 0;
								do
									stack[sptr++] = '0' + (temp % 10);
								while(temp = temp / 10);
								while(sptr)
									*outptr++ = stack[--sptr]; }
							else if(chr == '.') {
								temp = cur_line;
								goto write_num; }
							else if(chr == '@')			/* time/date info */
								switch(*macptr++) {
									case '1' :			/* day, no lead */
										temp = Da;
										goto write_num;
									case '2' :			/* day */
										temp = Da;
write_two:								if(temp < 10)
											*outptr++ = '0';
										goto write_num;
									case '3' :			/* month */
										temp = Mo;
										goto write_two;
									case '4' :			/* string month */
										parptr = months[Mo];
										while(*parptr)
											*outptr++ = *parptr++;
										break;
									case '5' :			/* string month (3) */
										parptr = months[Mo];
										*outptr++ = *parptr++;
										*outptr++ = *parptr++;
										*outptr++ = *parptr++;
										break;
									case '6' :			/* year */
										temp = Yr % 100;
										goto write_two;
									case '7' :			/* hour */
										temp = Hr;
										goto write_two;
									case '8' :			/* min. */
										temp = Mi;
										goto write_two;
									case '9' :			/* second */
										temp = Se;
										goto write_two;
									default :
										quit("Invalid TIME code"); }
							else {
								*outptr++ = '\\';
								*outptr++ = chr; } }
						*outptr++ = desize = 0;
						macptr = process_line(outbuf, macptr);
						if((macptr >= deptr) && (macptr < (buffer+MACSPACE)))
							macptr -= desize;
						if((mptr >= deptr) && (mptr < (buffer+MACSPACE)))
							mptr -= desize; }
					while(macptr < macend[i]);
					need_line = -1;
					break; } }
			if(i == minum) {
				if(need_line) {
					if(adjline)
						fprintf(ofp, " line %u\n", cur_line-1);
					need_line = 0; }
				fputs(linebuf, ofp);
				putc('\n', ofp);
				return mptr;} } }
	need_line = -1;
	return mptr;
}

/* determine if character in a terminator char */
int isend(chr)
	unsigned char chr;
{
	return (chr==' ') | (chr==0) | (chr==9) | (chr==',')
	     | (chr=='\n') | (chr=='\r');	/* fgets includes newline on Linux */
}

/* determine is char is alphanumeric */
int issymbol(chr)
	unsigned char chr;
{
	return isalpha(chr) | isdigit(chr) | (chr == '_');
}

/* skips blanks */
unsigned char *skip_blank(linptr)
	unsigned char *linptr;
{
	while((*linptr==' ')||(*linptr==9)) ++linptr;
	return linptr;
}

/* skips ahead to the n'th parameter */
unsigned char *skip_parm(n,ptr)
	unsigned n;
	unsigned char *ptr;
{
	while(n) {
		if(*ptr == '"') {
			++ptr;
			while(*ptr != '"') ++ptr; }
		while((*ptr) && (*ptr++ != ','));
		--n; }
	return ptr;
}

/* set the value for a symbol */
void set_symbol(symbol, value)
	unsigned char *symbol, *value;
{
	unsigned i;
	unsigned char *ptr, *ptr1;

	if(isend(*symbol))
		quit("Invalid SET symbol");

	/* Search for and delete existing symbol */
	for(i=0; i < msnum; ++i) {
		if(!strcmp(macsub[i], symbol)) {
			deptr = ptr = ptr1 = macsub[i];
			while(*ptr++);		/* Skip name */
			while(*ptr++);		/* Skip body */
			freptr -= (desize = ptr - ptr1);
			--msnum;
			while(i < msnum) {
				macsub[i] = macsub[i+1] - desize;
				++i; }
			for(i=0; i <= minum; ++i)
				if(macins[i] >= ptr1) {
					macins[i] -= desize;
					macend[i] -= desize; }
			while(ptr1 < freptr)
				*ptr1++ = *ptr++;
			break; } }

	/* Copy in symbol definition */
	if(msnum >= NUMSMACS)
		quit("Set MACRO pool exausted");
	macsub[msnum++] = freptr;
	while(*freptr++ = *symbol++);
	freptr = extract_parm(value, freptr);
	++freptr;
	check_free();
}

/* perform macro substution */
void macro_sub(ptr)
	unsigned char *ptr;
{
    unsigned i;
	unsigned char chr, flag, *outptr, *savptr, *tmptr, *txtptr, outbuf[LINSIZE];

	outptr = outbuf;
	savptr = ptr;
	flag = 0;

again:
	while(*ptr) {
		if(issymbol(*ptr)) {
			i = msnum;
			while(i) {
				tmptr = ptr;
				txtptr = macsub[--i];
				while((chr=*txtptr) && (toupper(*tmptr)==chr)) {
					++txtptr;
					++tmptr; }
				if((!*txtptr) && (!issymbol(*tmptr))) {	/* substitution occurs */
					ptr = tmptr;						/* pass substuted text */
					while(chr=*++txtptr) *outptr++ = chr;
					flag = -1;
					goto again; } }
			while(issymbol(*ptr))
				*outptr++ = *ptr++; }
		else
			*outptr++ = *ptr++; }

	if(flag) {
		*outptr++ = 0;
		outptr = outbuf;
		while(*savptr++ = *outptr++); }
}

unsigned char *extract_parm(ptr,dest)
	unsigned char *ptr, *dest;
{
	if(*ptr=='"') {
		++ptr;
		while(*ptr != '"') *dest++ = *ptr++; }
	else
		while(!isend(*ptr)) *dest++ = *ptr++;
	*dest = 0;
	return dest;
}

int parms_equal(ptr)
	unsigned char *ptr;
{
	extract_parm(ptr, instr);
	ptr = skip_parm(1,ptr);
	extract_parm(ptr,lable);

	return !strcmp(instr, lable);
}

void check_free()
{
	if((buffer+sizeof(buffer)) <= freptr)
		quit("MACRO storage pool exausted");
}

void quit(string)
	unsigned char *string;
{
	fprintf(stderr,"MACRO: Error in file '%s', line %u : %s\n",
		cur_file, cur_line, string);
	exit(-1);
}

/*
 * Get a numerical data value from the input stream
 */
unsigned get_value()
{
	unsigned num, b, i;
	unsigned char x[17], *ptr, c;

	num = i = 0;
	input_ptr = skip_blank(input_ptr);
DEBUG(("GET_VALUE: %s\n", input_ptr))
	switch(*input_ptr) {
		case '$' : b = 16; goto get_num;
		case '@' : b = 8; goto get_num;
		case '%' : b = 2; goto get_num; }
	if(isdigit(*input_ptr)) {
		b = 10;
		x[i++] = *input_ptr;
	get_num:
		while(isalnum(c = toupper(*++input_ptr)))
			x[i++] = (c < 'A') ? c : c - 7;
		switch(x[i-1]) {
			case 'H'-7: b = 16;	goto deci;
			case 'Q' :
			case 'O'-7: b = 8;	goto deci;
			case 'B'-7: b = 2;	goto deci;
			case 'T' :
			case 'D' : b = 10;
			deci: --i; }
			if(!i)
				quit("Invalid numeric constant");
			for(ptr = x; i; --i) {
				if((c = *ptr++ - '0') >= b)
					quit("Invalid numeric constant");
				num = (num * b) + c; }
		return num; }
	switch(*input_ptr++) {
		case '(' :	return expression();
		case '-' :	return -get_value();
		case '~' :	return ~get_value();
		case '!' :	return !get_value();
		case '\'' :		/* character value */
			while(*input_ptr != '\'') {
				if(!*input_ptr)
					quit("Unterminated string");
				num = (num << 8) + *input_ptr++; }
			++input_ptr;
			return num; }
	quit("Invalid constant in expression");
}

/*
 * Process a numerical expression in the input stream.
 */
unsigned expression()
{
	unsigned value;

DEBUG(("Expression: %s\n", input_ptr))
	value = get_value();
	for(;;) {
		input_ptr = skip_blank(input_ptr);
		switch(*input_ptr++) {
			case ')' :
			case '\0' :
				return value;
			case '+' : value += get_value();	break;
			case '-' : value -= get_value();	break;
			case '*' : value *= get_value();	break;
			case '/' : value /= get_value();	break;
			case '%' : value %= get_value();	break;
			case '&' :
				if(*input_ptr == '&') {
					++input_ptr;
					value = get_value() && value;
					break; }
				value &= get_value();			break;
			case '|' :
				if(*input_ptr == '|') {
					++input_ptr;
					value = get_value() || value;
					break; }
				value |= get_value();			break;
			case '^' : value ^= get_value();	break;
			case '>' :
				switch(*input_ptr++) {
					case '>' : value >>= get_value();			break;
					case '=' : value = value >= get_value();	break;
					default: --input_ptr; value = value > get_value(); }
				break;
			case '<' :
				switch(*input_ptr++) {
					case '<' : value <<= get_value();			break;
					case '=' : value = value <= get_value();	break;
					default: --input_ptr; value = value < get_value(); }
				break;
			case '=' :
				if(*input_ptr++ != '=')
					goto error;
				value = value == get_value();
				break;
			case '!' :
				if(*input_ptr++ == '=') {
					value = value != get_value();
					break; }
			default:
			error:
				quit("Invalid operator in expression"); } }
}
