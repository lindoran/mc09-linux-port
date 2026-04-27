/*
 * Unix I/O routines for the DDS MICRO-C compiler.
 *
 * These I/O routines should work on any "UNIX" system, or any
 * system supporting a "UNIX compatible" I/O library.
 *
 * To reduce the difficulty in porting to non-standard systems, these
 * routines use as few library functions as possible. There may be more
 * efficent ways to perform I/O in a given environment.
 *
 * For compilers which use multiple memory models (68HC16, 8051 & 8086), add
 * the line '#define MODEL' at the beginning of this file before compiling.
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "portab.h"

#ifndef LINE_SIZE
#include "compile.h"

/* Variables in the COMPILE.C module */
extern char line, comment, fold;	/* Command line switch flags */
extern char file_name[];			/* Filename table */
extern unsigned file_depth;			/* Include depth indicator */
#endif

FILE *fp_table[INCL_DEPTH+1] = { 0 }, *output_fp = 0;
char quiet = 0;

/* Include search path: set by -I option or MCINCLUDE env var */
static char include_path[FILE_SIZE] = "";

#ifdef MODEL			/* compiler's with multiple memory models only */
extern char model;					/* memory model to use */
#endif
extern char symbolic;				/* Symbolic output */
extern char nowarn;					/* Suppress unreferenced warnings */
extern unsigned max_errors;			/* Max errors before forced abort */

#ifndef CPU
#define CPU "PC86"
#endif

/* Proper stringize for CPU name (K&R ## not valid in modern cpp) */
#define STRINGIFY_(x) #x
#define STRINGIFY(x)  STRINGIFY_(x)

char hello[] = { "DDS MICRO-C " STRINGIFY(CPU) " Compiler\n\
?COPY.TXT 1988-2005 Dave Dunfield\n\
**See COPY.TXT**.\n" };

/*
 * Initialize I/O & execute compiler
 */
main(argc, argv)
	int argc;
	char *argv[];
{
	int i;
	char *ptr;
	int opt;

/* first process any filenames and command line options */
	for(i=1; i < argc; ++i) {
		ptr = argv[i];
		opt = (ptr[0] << 8) | ptr[1];
		ptr += 2;
		switch(opt) {
			case ('-'<<8)+'c' :		/* Include comments */
				comment = -1;
				break;
			case ('-'<<8)+'f' :		/* Fold literal strings */
				fold = -1;
				break;
			case ('-'<<8)+'l' :		/* Accept line numbers */
				line = -1;
				break;
			case ('-'<<8)+'q' :		/* Quiet mode */
				quiet = -1;
				break;
			case ('-'<<8)+'s' :		/* Symbolic output */
				symbolic = -1;
				break;
			case ('-'<<8)+'W' :		/* Suppress unreferenced warnings */
				nowarn = -1;
				break;
			case ('-'<<8)+'e' :		/* Max errors (-eN) */
				if(*ptr)
					max_errors = atoi(ptr);
				break;
			case ('-'<<8)+'I' :		/* Include path (-Ipath or -I path) */
				if(*ptr)
					strncpy(include_path, ptr, sizeof(include_path)-1);
				else if(i+1 < argc)
					strncpy(include_path, argv[++i], sizeof(include_path)-1);
				break;
#ifdef MODEL
			case ('m'<<8)+'=' :		/* memory model to use */
				model = *ptr - '0';
				break;
#endif
			default:
				if(!fp_table[0]) {		/* Input file */
					copy_string(file_name, argv[i]);
					if(!(fp_table[0] = fopen(argv[i], "r")))
						severe_error("Cannot open input file"); }
				else if(!output_fp) {	/* Output file */
					if(!(output_fp = fopen(argv[i], "w")))
						severe_error("Cannot open output file"); }
				else
					severe_error("Too many parameters"); } }

/* Any files not explicitly named default to standard I/O */
	if(!fp_table[0])			/* default to standard input */
		fp_table[0] = stdin;
	if(!output_fp)				/* default to standard output */
		output_fp = stdout;

	if(!quiet)
		put_str(hello, 0);

	compile();
}

/*
 * Terminate the compiler
 */
terminate(rc)
	int rc;
{
	if(output_fp)
		fclose(output_fp);
	exit(rc ? -2 : 0);
}

/*
 * Write a number to file
 */
put_num(value, file)
	unsigned value;
	unsigned file;
{
	char stack[12];		/* 10 digits covers UINT32_MAX; 12 for safety */
	register unsigned i;

	i = 0;
	do
		stack[i++] = (value % 10) + '0';
	while(value /= 10);

	while(i)
		put_chr(stack[--i], file);
}

/*
 * Write a string to device indicated by "file"
 * (0 = console, non-0 = output file)
 */
put_str(ptr, file)
	char *ptr;
	unsigned file;
{
	while(*ptr)
		put_chr(*ptr++, file);
}

/*
 * The following routines use the standard library functions:
 * 'fopen', 'fclose', 'getc' and 'putc'
 * You may have to change these routines when porting
 * to a non-standard environment.
 */

/*
 * Stack previous input file & open a new one
 */
f_open(name)
	char *name;
{
	FILE *fp;
	char clean[FILE_SIZE+1], fullpath[FILE_SIZE*2+2];
	char *p, *q;

	/* Strip angle-bracket or quote delimiters from include filename */
	q = name;
	if (*q == '<' || *q == '"') ++q;
	p = clean;
	while (*q && *q != '>' && *q != '"') *p++ = *q++;
	*p = 0;

	/* Try working directory first */
	if (fp = fopen(clean, "r")) {
		fp_table[file_depth+1] = fp;
		return -1; }

	/* Try -I path */
	if (include_path[0]) {
		snprintf(fullpath, sizeof(fullpath), "%s/%s", include_path, clean);
		if (fp = fopen(fullpath, "r")) {
			fp_table[file_depth+1] = fp;
			return -1; } }

	/* Try MCINCLUDE environment variable */
	{ char *env = getenv("MCINCLUDE");
	  if (env) {
		snprintf(fullpath, sizeof(fullpath), "%s/%s", env, clean);
		if (fp = fopen(fullpath, "r")) {
			fp_table[file_depth+1] = fp;
			return -1; } } }

	return 0;
}

/*
 * Close input file & return to last one
 */
f_close()
{
	fclose(fp_table[file_depth]);
}

/*
 * Read a line from the source file, return 1 if end of file,
 */
get_lin(line)
	char *line;
{
	register int chr, i;

	i = LINE_SIZE;
	while(--i) {
		if((chr = getc(fp_table[file_depth])) < 0) {
			if(i == (LINE_SIZE-1))
				return -1;
			break; }
		if(chr == '\n')
			break;
		if(chr == '\r')   /* strip CR from CRLF source files */
			continue;
		*line++ = chr; }
	return *line = 0;
}

/*
 * Write character to device indicated by "file"
 * (0 = console, non-0 = output file)
 */
put_chr(chr, file)
	char chr;
	char file;
{
	putc(chr, file ? output_fp : stderr);
}
