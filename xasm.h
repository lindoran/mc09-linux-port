/*
 * XASM parameters and support routines
 *
 * ?COPY.TXT 1983-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

/* Linux portability - must precede stdio.h */
#include "portab.h"

#define	LINE_SIZE	250			/* Maximum size of an input line */
#define	SYMB_SIZE	24			/* Maximum width of a symbol */
#ifdef DEMO
	#define	SYMB_POOL	1200
	#define	DEMO_LINES	1000
	#define	DEMO_HEX	2048
#else
	#define	SYMB_POOL	45000	/* Maximum size of symbol memory pool */
#endif

/* Flags in upper four bits of symbol table prefix entry */
#define SYMSORT		0x80	/* Symbol has been sorted */
#define	SYMMASK		0x1F	/* Mask for symbol length */

#ifdef DEMO
	#include "c:\project\demonote.txt"
	int demo_line = 0, demo_hcount = 0;
	xabort(msg)
		char *msg;
	{
		int i;
		char *ptr;

		fprintf(stderr,"\nDemo version aborted because of too many %ss!\n", msg);
		fputs(demo_text, stderr);
		exit(-1);
	}
	xhex(c)
		int c;
	{
		if((demo_hcount += c) > DEMO_HEX)
			xabort("output byte");
	}
#else
	#define xabort(msg)
	#define	xhex(c)
#endif

/*
 * Get a line of input from a file, and return it as a NULL
 * terminated string WITHOUT a trailing newline character.
 */
char *MC_fgets(buffer, max_len, fp)
	char *buffer;
	int max_len;
	FILE *fp;
{
	register int i, c = 0;
	register char *ptr;

	i = 0;
	ptr = buffer;
	while(i < max_len) {
		if((c = getc(fp)) == -1) {
#ifdef DEMO
			demo_line = -1;
#endif
			break; }
		if(c == '\n')
			break;
		if(c == '\r')   /* strip CR from CRLF source files */
			continue;
		*ptr++ = c;
		++i;; }

	*ptr = 0;

#ifdef DEMO
	if(++demo_line >= DEMO_LINES)
		xabort("line");
#endif
	return ((c != '\n') && !i) ? 0 : buffer;
}
