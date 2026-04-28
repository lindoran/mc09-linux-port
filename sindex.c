/*
 * DDS MICRO-C Source Indexer
 *
 * This program help with the generation of the "EXTINDEX.LIB" file
 * for the SLINK utility. All files matching the specifed pattern
 * are examined, and entries are made for each file, containing all
 * non-compiler generated labels.
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 *
 * Compile command: cc sindex -fop
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glob.h>

#include "microc.h"
#include "portab.h"

#define	LINESIZ		100		/* Maximum size of input line */
#define	FILESIZ		12		/* Maximum size of file name */

int main(int argc, char *argv[])
{
	int i;
	char buffer[LINESIZ+1], *ptr, *ptr1;
	FILE *index_fp, *fp;
	static char *pattern = "*.ASM", *index = "EXTINDEX.LIB";

	fputs("DDS MICRO-C Source Indexer\n?COPY.TXT 1990-2005 Dave Dunfield\n**See COPY.TXT**.\n", stderr);

	for(i=1; i < argc; ++i) {
		char *_a = argv[i];
		if(_a[0]=='?') { argc = 0; }
		else if(_a[0]=='i' && _a[1]=='=') { index = &_a[2]; }
		else pattern = _a; }

	if(!argc)
		die("\nUse SINDEX [filespec i=index]\n");

	{ glob_t gl; int gi;
	  if(glob(pattern, 0, 0, &gl) || !gl.gl_pathc) {
	  	fprintf(stderr,"No files matching: %s\n",pattern);
		exit(-1); }
	  index_fp = fopen(index, "w");
	  for(gi = 0; gi < (int)gl.gl_pathc; ++gi) {
		char *gname = gl.gl_pathv[gi];
		if((fp = fopen(gname, "r"))) {
			fprintf(stderr,"Processing: %s\n",gname);
			putc('-', index_fp);
			fputs(gname, index_fp);
			putc('\n', index_fp);
			while(fgets(ptr = buffer, LINESIZ, fp)) {
				if(!strncmp(buffer, "$DD:", 4)) ptr += 4;
				if(isalpha(*ptr) || (*ptr == '_')) {
					ptr1 = ptr;
					while(isalnum(*ptr1) || (*ptr1 == '_')) ++ptr1;
					if(isspace(*ptr1) || !*ptr1) {
						*ptr1 = 0;
						fputs(ptr, index_fp);
						putc('\n', index_fp); } } }
			fclose(fp); } }
	  globfree(&gl); }

	fclose(index_fp);
}
