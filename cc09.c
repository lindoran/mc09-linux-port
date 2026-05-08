/*
 * DDS MICRO-C Command Coordinator
 *
 * This program integrates the MCP, MCC09, MCO09, SLINK and ASM09
 * operations into a single command.
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>

/* Dunfield getenv(name, buf) -> POSIX getenv() returning into buf */
static int mc_getenv(char *name, char *buf) {
    char *v = getenv(name);
    if (!v) return 0;
    strcpy(buf, v);
    return 1;
}
/* Single-arg getenv returning char* for internal use */
static char *mc_getenv_str(char *name) { return getenv(name); }
#define getenv(n,b) mc_getenv((n),(b))

/* Dunfield exec(cmd, args_str) - run cmd with space-separated args */
static int mc_exec(char *cmd, char *args) {
    char *argv[64]; int ac = 0;
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s %s", cmd, args);
    /* tokenize */
    char *p = buf;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;
        argv[ac++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = 0;
    }
    argv[ac] = NULL;
    pid_t pid = fork();
    if (pid == 0) { execvp(argv[0], argv); exit(2); }
    int st; waitpid(pid, &st, 0);
    if (WIFEXITED(st)) return WEXITSTATUS(st);
    return -1;
}
#define exec(cmd,args) mc_exec((cmd),(args))

#define	NOFILE	2		/* EXEC return code for file not found */
#define	NOPATH	3		/* EXEC return code for path not found */

char mcdir[256], libdir[256], temp[256], ofile[256], tail[1024], mcparm[256];
char do_link = -1, opt = 0, pre = 0, xasm = -1, verb = -1, intel = 0,
	lst = 0, del = -1, com = 0, macro = 0, fold = 0, symb = 0,
	do_dup = 0, *fnptr, *mptr = mcparm, *startup = 0;

char htext[] = { "\n\
Use: CC09 <name> [-acdfiklmopqsx h= s= t=] [symbol=value]\n\n\
opts:	-Asm		-Comment	-Dupwarn	-Foldliteral\n\
	-Intelhex	-Keeptemp	-Listing	-Macro\n\
	-Optimize	-Preprocess	-Quiet		-Symbolic\n\
	-eXtended-asm\n\n\
	H=homepath	S=startup	T=temprefix\n\
\n\?COPY.TXT 1990-2005 Dave Dunfield\n**See COPY.TXT**.\n" };

/*
 * Main program, process options & invoke appropriate commands
 */
main(argc, argv)
	int argc;
	char *argv[];
{
	int i;
	char ifile[256], *ptr, c;
	char incbuf[256];

	/* Get default directories from environment */
	if(!getenv("MCDIR", mcdir))	{	/* Get MICRO-C directory */
		message("Environment variable MCDIR is not set!\n\n");
		strcpy(mcdir,"./"); }
	if(!getenv("MCTMP", temp)) {	/* Get temporary directory */
		if(getenv("TEMP", temp)) {
			if(temp[(i = strlen(temp))-1] != '/') {
				temp[i] = '/';
				temp[i+1] = 0; } } }

	/* Set library directory (MCLIBDIR overrides mcdir/lib09) */
	if(!getenv("MCLIBDIR", libdir)) {
		snprintf(libdir, sizeof(libdir), "%s/lib09", mcdir); }

	/* parse for command line options. */
	for(i=2; i < argc; ++i) {
		if(*(ptr = argv[i]) == '-') {		/* Enable switch */
			while(*++ptr) {
				switch(toupper(*ptr)) {
					case 'A' : do_link = 0;		continue;
					case 'C' : com = -1;		continue;
					case 'F' : fold = -1;		continue;
					case 'I' : intel = -1;		continue;
					case 'K' : del = 0;			continue;
					case 'L' : lst = -1;		continue;
					case 'M' : macro = -1;		continue;
					case 'O' : opt = -1;		continue;
					case 'D' : do_dup = -1;
					case 'P' : pre = -1;		continue;
					case 'Q' : verb = 0;		continue;
					case 'S' : symb = -1;		continue;
					case 'X' : xasm = 0;		continue; }
				goto badopt; }
			continue; }

		if(*(ptr+1) == '=') switch(toupper(*ptr)) {
			case 'H' : strcpy(mcdir, ptr+2);	continue;
			case 'S' : startup = ptr+2;			continue;
			case 'T' : strcpy(temp, ptr+2);		continue; }

		*mptr++ = ' ';
		c = 0;
		while(*mptr++ = *ptr++) {
			if(*ptr == '=')
				c = pre; }
		if(c)
			continue;

	badopt:
		fprintf(stderr,"Invalid option: %s\n", argv[i]);
		exit(-1); }

	message("DDS MICRO-C 6809 Cross Compiler v3.23\n");

	if(argc < 2) {
		fputs(htext, stderr);
		exit(-1); }

	/* Parse filename & extension from passed path etc. */
	fnptr = ptr = argv[1];
	while(c = *ptr) {
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
		{ char *_inc = mc_getenv_str("MCINCLUDE");
		  if(!_inc) {
			snprintf(incbuf, sizeof(incbuf), "%s%sinclude",
				mcdir, (mcdir[strlen(mcdir)-1] == '/') ? "" : "/");
			_inc = incbuf; }
		  snprintf(tail, sizeof(tail), "%s %s -I%s -q%s%s",
			ifile, ofile, _inc, do_dup ? " -d" : "", mcparm); }
		docmd("mcp");
		strcpy(ifile, ofile); }

	/* Compile to assembly language */
	next_step("Compile... ", opt||macro||do_link);
	snprintf(tail, sizeof(tail), "%s %s -q%s%s%s%s", ifile, ofile,
		pre ? " -l" : "",	com ? " -c" : "", fold ? " -f" : "",
		symb ? " -s" : "");
	docmd("mcc09");
	if(pre)
		erase(ifile);
	strcpy(ifile, ofile);

	/* Optimize the assembly language */
	if(opt) {
		next_step("Optimize... ", macro||link);
		snprintf(tail, sizeof(tail), "%s %s -q", ifile, ofile);
		docmd("mco09");
		erase(ifile);
		strcpy(ifile, ofile); }

	/* Run assembler MACRO processor */
	if(macro) {
		next_step("Macro... ", link);
		snprintf(tail, sizeof(tail),"%s >%s", ifile, ofile);
		docmd("macro");
		erase(ifile);
		strcpy(ifile, ofile); }

	/* Execute the SOURCE LINKER */
	if(do_link) {
		next_step("Link... ", xasm);
		sprintf(mcparm, startup ? " S=%s" : "", startup);
		snprintf(tail, sizeof(tail), "%s %s t=%s l=%s -q%s%s",
			ifile, ofile, temp, libdir, symb ? " -s" : "", mcparm);
		docmd("slink");
		erase(ifile);
		strcpy(ifile, ofile);

	/* Assemble into object module */
		if(xasm) {
			message("Assemble...\n");
			snprintf(tail, sizeof(tail), "%s c=%s l=%s -c%s%s%s", ifile, fnptr, fnptr,
				verb ? "" : "q", intel ? "i" : "", lst ? "fs" : "t");
			docmd("asm09");
			erase(ifile); } }

	message("All done.\n");
}

/*
 * Execute a command, looking for it in the MICRO-C directory,
 * and also in any directories found in the PATH environment
 * variable. Operands to the command have been previously
 * defined in the global variable 'tail'.
 */
docmd(char *cmd)
{
	int rc;
	char command[512], *ptr, *ptr1, c;
	static char path[2000];

	ptr = mcdir;						/* First try MC home dir */
	if(!getenv("PATH", ptr1 = path))	/* And then search  PATH */
		ptr1 = "";

	do {	/* Search MCDIR & PATH for commands */
		int need_slash = *ptr && (ptr[strlen(ptr)-1] != '/');
		snprintf(command, sizeof(command), "%s%s%s",
			ptr, need_slash ? "/" : "", cmd);
		rc = exec(command, tail);
		ptr = ptr1;						/* Point to next directory */
		while(c = *ptr1) {				/* Advance to end of entry */
			++ptr1;
			if((c == ';') || (c == ':')) {
				*(ptr1 - 1) = 0;		/* Zero terminate */
				break; } } }
	while(((rc == NOFILE) || (rc == NOPATH)) && *ptr);
	if(rc) {
		fprintf(stderr,"%s failed (%d)\n", cmd, rc);
		exit(-1); }
}

/*
 * Output an informational message (verbose mode only)
 */
message(ptr)
	char *ptr;
{
	if(verb)
		fputs(ptr, stderr);
}

/*
 * Erase temporary file (if enabled)
 */
erase(file)
	char *file;
{
	if(del)
		remove(file);
}

/*
 * Create a new output file name (permanent or temporary)
 */
next_step(msg, flag)
	char *msg;
	int flag;
{
	static int tnum = 0;

	message(msg);

	if(flag)
		snprintf(ofile, sizeof(ofile), "%s%s.%u", temp, fnptr, ++tnum);
	else
		sprintf(ofile,"%s.asm", fnptr);
}
