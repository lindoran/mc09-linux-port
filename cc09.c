/*
 * DDS MICRO-C Command Coordinator
 *
 * This program integrates the MCP, MCC09, MCO09, SLINK and ASM09
 * operations into a single command.
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#ifdef __unix__
#  include <unistd.h>
#else
#  error Define access() for your system
#endif

/* Prototype required functions */
static void message  (char const *);
static void next_step(char const *,int);
static bool docmd    (char const *);

static char tlibdir[FILENAME_MAX];
static char ofile  [FILENAME_MAX];
static char tail   [BUFSIZ];
static char mcparm [BUFSIZ];

static char const *mcdir;
static char const *temp;
static char const *libdir;
static char       *mptr = mcparm;
static char       *startup;
static char       *fnptr;

static bool do_link = true;
static bool opt     = false;
static bool pre     = false;
static bool xasm    = true;
static bool verb    = true;
static bool intel   = false;
static bool lst     = false;
static bool del     = true;
static bool com     = false;
static bool macro   = false;
static bool fold    = false;
static bool symb    = false;
static bool do_dup  = false;

static char htext[] = { "\n\
Use: CC09 <name> [-acdfiklmopqsx h= s= t=] [symbol=value]\n\n\
opts:	-Asm		-Comment	-Dupwarn	-Foldliteral\n\
	-Intelhex	-Keeptemp	-Listing	-Macro\n\
	-Optimize	-Preprocess	-Quiet		-Symbolic\n\
	-eXtended-asm\n\n\
	H=homepath	S=startup	T=temprefix\n\
\n\?COPY.TXT 1990-2005 Dave Dunfield\n**See COPY.TXT**.\n" };

static bool mc_exec(char *cmd, char *args) {
    char buffer[BUFSIZ];
    int rc;
    
    snprintf(buffer,sizeof(buffer),"%s %s",cmd,args);
    if ((rc = system(buffer)) != 0)
      fprintf(stderr,"%s failed: %d\n",cmd,rc);
    return rc == 0;
}

/*
 * Main program, process options & invoke appropriate commands
 */
int main(int argc, char *argv[])
{
	int i;
	char ifile[256], *ptr, c;
	char incbuf[256];
	bool cmd;

	/* Get default directories from environment */
	mcdir = getenv("MCDIR"); /* Get MICRO-C directory */
	if (mcdir == NULL) {
		message("Environment variable MCDIR is not set!\n\n");
		mcdir = "."; }
	temp = getenv("MCTMP"); /* Get temporary directory */
	if (temp == NULL) {
	  temp = getenv("TMPDIR"); /* typical name under Unix */
	  if (temp == NULL)
	    temp = "/tmp";
	}
	libdir = getenv("MCLIBDIR"); /* set library directory (MCLIBDIR overrides mcdir/lib09 */
	if (libdir == NULL)
	{
	  snprintf(tlibdir,sizeof(tlibdir),"%s/lib09",mcdir);
	  libdir = tlibdir;
	}
	
	/* parse for command line options. */
	for(i=2; i < argc; ++i) {
		if(*(ptr = argv[i]) == '-') {		/* Enable switch */
			while(*++ptr) {
				switch(toupper(*ptr)) {
					case 'A' : do_link = false; continue;
					case 'C' : com     = true;  continue;
					case 'F' : fold    = true;  continue;
					case 'I' : intel   = true;  continue;
					case 'K' : del     = false; continue;
					case 'L' : lst     = true;  continue;
					case 'M' : macro   = true;  continue;
					case 'O' : opt     = true;  continue;
					case 'D' : do_dup  = true;  continue;
					case 'P' : pre     = true;  continue;
					case 'Q' : verb    = false; continue;
					case 'S' : symb    = true;  continue;
					case 'X' : xasm    = false; continue; }
				goto badopt; }
			continue; }

		if(*(ptr+1) == '=') switch(toupper(*ptr)) {
			case 'H' : mcdir   = ptr+2; continue;
			case 'S' : startup = ptr+2; continue;
			case 'T' : temp    = ptr+2; continue; }

		*mptr++ = ' ';
		c = 0;
		while((*mptr++ = *ptr++)) {
			if(*ptr == '=')
				c = pre; }
		if(c)
			continue;

	badopt:
		fprintf(stderr,"Invalid option: %s\n", argv[i]);
		return 1; }

	message("DDS MICRO-C 6809 Cross Compiler v3.23\n");

	if(argc < 2) {
		fputs(htext, stderr);
		return 1; }

	/* Parse filename & extension from passed path etc. */
	fnptr = ptr = argv[1];
	while((c = *ptr)) {
		if(c == '.')
			goto noext;
		++ptr;
		if((c == ':') || (c == '/'))
			fnptr = ptr; }
	strcpy(ptr, ".c");
noext:
	strcpy(ifile, argv[1]);
	message(fnptr);
	message(": ");
	*mptr = *ptr = 0;

	/* Pre-process to source file */
	if(pre) {
		next_step("Preprocess... ", -1);
		{ char *_inc = getenv("MCINCLUDE");
		  if(!_inc) {
			snprintf(incbuf, sizeof(incbuf), "%s/%s/include",
				mcdir, (mcdir[strlen(mcdir)-1] == '/') ? "" : "/");
			_inc = incbuf; }
		  snprintf(tail, sizeof(tail), "%s %s -I%s -q%s%s",
			ifile, ofile, _inc, do_dup ? " -d" : "", mcparm); }
		if (!docmd("mcp"))
		{
		  message("failed\n");
		  return 1;
		}
		strcpy(ifile, ofile); }

	/* Compile to assembly language */
	next_step("Compile... ", opt|macro|do_link);
	snprintf(tail, sizeof(tail), "%s %s -q%s%s%s%s", ifile, ofile,
		pre ? " -l" : "",	com ? " -c" : "", fold ? " -f" : "",
		symb ? " -s" : "");
	cmd = docmd("mcc09");
	if(pre)
		remove(ifile);
	if (!cmd)
	{
	  message("failed\n");
	  return 1;
	}
	strcpy(ifile, ofile);

	/* Optimize the assembly language */
	if(opt) {
		next_step("Optimize... ", macro|do_link);
		snprintf(tail, sizeof(tail), "%s %s -q", ifile, ofile);
		cmd = docmd("mco09");
		remove(ifile);
		if (!cmd)
		{
		  message("failed\n");
		  return 1;
		}
		strcpy(ifile, ofile); }

	/* Run assembler MACRO processor */
	if(macro) {
		next_step("Macro... ", do_link);
		snprintf(tail, sizeof(tail),"%s >%s", ifile, ofile);
		cmd = docmd("macro");
		remove(ifile);
		if (!cmd)
		{
		  message("failed\n");
		  return 1;
		}
		strcpy(ifile, ofile); }

	/* Execute the SOURCE LINKER */
	if(do_link) {
		next_step("Link... ", xasm);
		snprintf(mcparm, sizeof(mcparm),startup ? " S=%s" : "", startup);
		snprintf(tail, sizeof(tail), "%s %s t=%s l=%s -q%s%s",
			ifile, ofile, temp, libdir, symb ? " -s" : "", mcparm);
		cmd = docmd("slink");
		remove(ifile);
		if (!cmd)
		{
		  message("failed\n");
		  return 1;
		}
		strcpy(ifile, ofile);

	/* Assemble into object module */
		if(xasm) {
			message("Assemble...\n");
			snprintf(tail, sizeof(tail), "%s c=%s l=%s -c%s%s%s", ifile, fnptr, fnptr,
				verb ? "" : "q", intel ? "i" : "", lst ? "fs" : "t");
			cmd = docmd("asm09");
			remove(ifile);
			if (!cmd)
			{
			  message("failed\n");
			  return 1; } } }

	message("All done.\n");
	return 0;
}

/*
 * Execute a command, looking for it in the MICRO-C directory,
 * and also in any directories found in the PATH environment
 * variable. Operands to the command have been previously
 * defined in the global variable 'tail'.
 */

#if defined(_WIN32)
#  define PATH_SEPARATOR ';'
#else
#  define PATH_SEPARATOR ':'
#endif

static bool docmd(char const *cmd)
{
  char  program[FILENAME_MAX];
  char *path;
  char *ps;
  int   len;
  
  /*------------------------------------------
  ; try $MCDIR/cmd first.  If okay, return
  ;-------------------------------------------*/
  
  snprintf(program,sizeof(program),"%s/%s",mcdir,cmd);
  if (access(program,X_OK) == 0)
    return mc_exec(program,tail);
    
  /*----------------------
  ; now try $PATH
  ;-----------------------*/
  
  path = getenv("PATH");
  if (path == NULL)
    path = "./";
    
  do
  {
    ps = strchr(path,PATH_SEPARATOR);
    if (ps == NULL)
      ps = strchr(path,'\0');
    assert(ps != NULL);
    len = (int)(ps - path);
    
    snprintf(program,sizeof(program),"%.*s/%s",len,path,cmd);
    if (access(program,X_OK) == 0)
      return mc_exec(program,tail);
      
    path = ps + 1;
  } while(*ps != '\0');
  
  return false;
}

/*
 * Output an informational message (verbose mode only)
 */
static void message(char const *ptr)
{
	if(verb)
		fputs(ptr, stderr);
}

/*
 * Create a new output file name (permanent or temporary)
 */
static void next_step(char const *msg, int flag)
{
	static int tnum = 0;

	message(msg);

	if(flag)
		snprintf(ofile, sizeof(ofile), "%s/%s.%u", temp, fnptr, ++tnum);
	else
		snprintf(ofile, sizeof(ofile), "%s.asm", fnptr);
}
