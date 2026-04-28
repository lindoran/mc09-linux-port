/*
 * DDS MICRO-C Code Generator for: 6809
 *
 *    The 6809 represents one of the most modern and powerful 8 bit
 * processors available. Its flexible addressing modes and ability to
 * perform 16 bit operations make it an ideal MICRO-C platform.
 *
 *    This code generator demonstrates a fairly complete implementation
 * of MICRO-C on the 6809. Although runtime library routines are used
 * for some common functions, most of the generated code is "inline",
 * resulting in fast execution.
 *
 * 6809 Registers:
 *	D (A,B)	- Accumulator
 *	U		- Index register
 *	X		- Second parameter to runtime library calls
 *	Y		- Not used.
 *	DP		- Not used.
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

#include <stdbool.h>

#ifndef LINE_SIZE
#include "portab.h"
#include "compile.h"
#include "6809cg.h"

extern void put_num(unsigned int,unsigned int);
extern void put_str(char *,unsigned int);
extern void put_chr(char,char);

extern char s_name[MAX_SYMBOL][SYMBOL_SIZE+1];
extern unsigned s_type[], s_index[], s_dindex[], dim_pool[], global_top,
	local_top, function_count;
#endif

unsigned stack_frame, stack_level, global_width = 0, global_set = 0;
char call_buffer[50], zero_flag, last_byte = 0;

char symbolic = 0;			/* Controls output of symbolic information */

/*
 * Generate a call to a runtime library function.
 */
char *runtime(char *string, unsigned int type, char order)
{
	char *ptr, *ptr1;

	if((type & (BYTE | POINTER)) == BYTE)		/* byte access */
		ptr = (type & UNSIGNED) ? "TFR D,X\n LDB |\n CLRA\n"
			: "TFR D,X\n LDB |\n SEX\n";
	else
		ptr = (order) ? "TFR D,X\n LDD |\n" : "LDX |\n";

	for(ptr1 = call_buffer; *ptr; ++ptr1)
		*ptr1 = *ptr++;
	*ptr1++ = ' ';
	*ptr1++ = 'J';
	*ptr1++ = 'S';
	*ptr1++ = 'R';
	*ptr1++ = ' ';
	while(*string)
		*ptr1++ = *string++;
	*ptr1 = 0;
	return call_buffer;
}

/*
 * Write an operand value
 * flag: (0=16 bit, 1=first 8 bits, 2 = second 8 bits)
 */
void write_oper(unsigned int token, unsigned int value, unsigned int type, unsigned int flag)
{
	int flag1;

	flag1 = (flag == 2);

	switch(token) {
		case NUMBER:
			if(flag == 1)
				value >>= 8;
			else if(flag1)
				value &= 0xff;
			out_chr('#');
			out_num(value);
			break;
		case STRING:
			out_str("#?0+");
			out_num(value);
			break;
		case SYMBOL:
			if((value < global_top) || (type & (STATIC|EXTERNAL))) {
				out_symbol(value);
				if(flag1)
					out_str("+1");
				break; }
			if(type & ARGUMENT) {
				if((type & (BYTE|POINTER|ARRAY)) == BYTE) /* 8of16 byte 2 */
					flag1 = 1;
				flag1 += stack_frame + 2; }
			out_num(stack_level + s_index[value] + flag1);
			out_str(",S");
			break;
		case IN_TEMP:
			out_str(flag1 ? "?temp+1" : "?temp");
			break;
		case INDIRECT:
			out_str(flag1 ? "1,U" : ",U");
			break;
		case ON_STACK:
			if(flag) {
				out_str(",S+");
				stack_level -= 1; }
			else {
				out_str(",S++");
				stack_level -= 2; }
			break;
		case ION_STACK:
			out_str("[,S++]");
			stack_level -= 2;
			break;
		case ISTACK_TOP:
			out_str("[,S]");
			break;
		default:		/* Unknown (error) */
			out_num(token);
			out_chr('?'); }
}

/*
 * Determine if a type is a pointer to 16 bits
 */
bool isp16(unsigned int type)
{
	if(type & (POINTER-1))			/* pointer to pointer */
		return true;
	if(type & POINTER)				/* first level pointer */
		return !(type & BYTE);
	return false;						/* not a pointer */
}

/*
 * Output text as comment in ASM source
 */
void do_comment(char *ptr)
{
	if(global_width) {
		out_nl();
		global_width = 0; }
	if(ptr) {
		out_chr('*');
		do_asm(ptr); }
}

/*
 * Output a string to the assembler followed by newline.
 */
void do_asm(char *ptr)
{
	out_str(ptr);
	out_nl();
}

/*
 * Release stack allocation
 */
void release_stack(unsigned int size)
{
	stack_level -= size;
}

/*
 * Define beginning of module
 */
void def_module(void) { }	/* No operation required */

/*
 * End of module definition
 */
void end_module(void)
{
	unsigned i;
	if(symbolic) for(i=0; i < global_top; ++i) {
		out_str("*#gbl ");
		dump_symbol(i); }
}

/*
 * Begin definition of a static global variable
 */
void def_static(unsigned int symbol, int ssize)
{
	(void)ssize; /* unused */
	out_symbol(symbol);
}

/*
 * Initialize static storage
 */
void init_static(unsigned int token, unsigned int value, char word)
{
	char *ptr;

	out_str(global_width ? "," : word ? " FDB " : " FCB ");
	switch(token) {
		case SYMBOL :		/* Symbol address */
			out_symbol(value);
			global_width += 12;
			break;
		case STRING :		/* literal pool entry */
			ptr = "?0+";
			global_width += 9;
			goto doinit;
		case LABEL :		/* instruction label */
			ptr = "?";
			global_width += 4;
			goto doinit;
		default :			/* constant value */
			ptr = "";
			global_width += 6;
		doinit:
			out_str(ptr);
			out_num(value); }

	if(global_width > 60) {
		global_width = 0;
		out_nl(); }

	++global_set;
}

/*
 * End static storage definition
 */
void end_static(void)
{
	if(global_width)
		out_nl();

	if(!global_set)
		out_inst("RMB 0");

	global_set = global_width = 0;
}

/*
 * Define a global variable
 */
void def_global(unsigned int symbol, unsigned int size)
{
	/* Output the special comment used by SLINK */
	out_str("$DD:");
	out_symbol(symbol);
	out_sp();

	/* If not using SLINK, remove above and uncomment this
	out_symbol(symbol);
	out_str(" RMB "); */

	out_num(size);
	out_nl();
}

/*
 * Define an external variable
 * In this case, we will just generate a comment, which can
 * be used by the SLINK utility to key external references.
 */
void def_extern(unsigned int symbol)
{
	out_str("$EX:");
	out_symbol(symbol);
	out_nl();
}

/*
 * Enter function & allocate local variable stack space
 */
void def_func(unsigned int symbol, unsigned int size)
{
	if(symbolic) {
		out_str("*#fun ");
		out_symbol(symbol);
		out_sp();
		put_num(size, -1);
		out_str(" ?");
		out_num(function_count);
		out_nl(); }

	out_symbol(symbol);
	if((stack_frame = size)) {
		out_str(" LEAS -");
		out_num(size);
		do_asm(",S"); }
	else
		out_inst("EQU *");
	stack_level = 0;
}

/*
 * Clean up the stack & end function definition
 */
void end_func(void)
{
	unsigned i;

	if(stack_frame || stack_level) {
		out_str(" LEAS ");
		out_num(stack_frame + stack_level);
		do_asm(",S"); }
	out_inst("RTS");

	if(symbolic) {
		for(i = local_top; i < MAX_SYMBOL; ++i) {
			out_str("*#lcl ");
			dump_symbol(i); }
		do_comment("#end"); }
}

/*
 * Dump a symbol definition "magic comment" to the output file
 */
void dump_symbol(unsigned int s)
{
	unsigned i, t;

	*s_name[s] &= 0x7F;
	out_symbol(s);
	out_sp();
	put_num(t = s_type[s], -1);
	out_sp();
	i = s_dindex[s];
	switch(t & SYMTYPE) {
		case FUNCGOTO :
			if(s < local_top)
				goto dofunc;
		case STRUCTURE :
			put_num(i, -1);
			break;
		default:
		dofunc:
			put_num(s_index[s], -1);
			if(t & ARRAY) {
				out_sp();
				put_num(t = dim_pool[i], -1);
				while(t--) {
					out_sp();
					put_num(dim_pool[++i], -1); } } }
	out_nl();
}

/*
 * Define a compiler generated label
 */
void def_label(unsigned int label)
{
	out_chr('?');
	out_num(label);
	out_inst("EQU *");
}

/*
 * Define literal pool
 */
void def_literal(char *ptr, unsigned int size)
{
	unsigned i;

	if(size) {
		i = 0;
		out_str("?0");
		while(i < size) {
			out_str((i % 16) ? "," : " FCB ");
			out_num(*ptr++);
			if(!(++i % 16))
				out_nl(); }
		if(i % 16)
			out_nl(); }
}

/*
 * Call a function by name
 */
void call(unsigned int token, unsigned int value, unsigned int type, unsigned int clean)
{
	out_str(" JSR ");
	switch(token) {
		case NUMBER :
			out_num(value);
			break;
		case SYMBOL :
			write_oper(token, value, type, 0);
			break;
		case ON_STACK :
		case ION_STACK :
			out_chr('[');
			if(clean) {
				out_num(clean++ *2);
				out_str(",S]"); }
			else {
				out_str(",S++]");
				stack_level -= 2; } }
	out_nl();

	if(clean += clean) {	/* clean up stack following function call */
		out_str(" LEAS ");
		out_num(clean);
		do_asm(",S");
		stack_level -= clean; }

	last_byte = (type & (BYTE | POINTER)) == BYTE;
	zero_flag = -1;
}

/*
 * Unconditional jump to label
 */
void jump(unsigned int label, bool ljmp)
{
	out_str(ljmp ? " JMP ?" : " BRA ?");
	out_num(label);
	out_nl();
}

/*
 * Conditional jump to label
 */
void jump_if(char cond, unsigned int label, char ljmp)
{
	char *ptr;

	if(zero_flag) {		/* set up 'Z' flag if necessary */
		out_inst(last_byte ? "TSTB" : "CMPD #0");
		zero_flag = 0; }

	if(cond)			/* jump if TRUE */
		ptr = ljmp ? " LBNE ?" : " BNE ?";
	else				/* jump if FALSE */
		ptr = ljmp ? " LBEQ ?" : " BEQ ?";

	out_str(ptr);
	out_num(label);
	out_nl();
}

/*
 * Perform a switch operation
 */
void do_switch(unsigned int label)
{
	out_str(" LDU #?");
	out_num(label);
	do_asm("\n JMP ?switch");
}

/*
 * Load index register with a pointer value
 */
void index_ptr(unsigned int token, unsigned int value, unsigned int type)
{
	if(token == IN_ACC)
		out_inst("TFR D,U");
	else {
		out_str(" LDU ");
		write_oper(token, value, type, 0);
		out_nl(); }
}

/*
 * Load index register with the address of an assignable object
 */
void index_adr(unsigned int token, unsigned int value, unsigned int type)
{
	if(token == ION_STACK) {
		out_inst("PULS U");
		stack_level -= 2; }
	else {
		out_str(((value < global_top) || (type & (STATIC|EXTERNAL))) ?
			" LDU #" : " LEAU ");
		write_oper(token, value, type, 0);
		out_nl(); }
}

/*
 * Expand 8 bit accumulator to 16 bits if necessary.
 */
void expand(unsigned int type)
{
	if(last_byte) {
		out_inst((type & UNSIGNED) ? zero_flag = -1, "CLRA" : "SEX");
		last_byte = 0; }
}

/*
 * Mark the accumulator as holding a byte-wide result without emitting code.
 *
 * Used by the cast handler in compile.c when an explicit (char) cast is
 * applied to a value that was loaded as a 16-bit integer.  On the 6809,
 * D = A:B; after LDD the low byte is already in B.  Setting last_byte here
 * causes the subsequent expand() call to emit SEX (signed char) or CLRA
 * (unsigned char) to produce the correct 16-bit sign/zero-extended result.
 *
 * No instruction is emitted — B already holds the correct low 8 bits.
 */
void narrow_byte(void)
{
	last_byte = -1;
}

/*
 * Do a simple register operation
 */
void accop(unsigned int oper, unsigned int rtype)
{
	char *ptr, byte, eflag, zflag;

	eflag = byte = zflag = 0;

	if((rtype & (BYTE | POINTER)) == BYTE)
		byte = -1;

	switch(oper) {
		case _STACK:			/* stack accumulator */
			eflag = -1;
			ptr = "PSHS A,B";
			stack_level += 2;
			zflag = 0x55;
			break;
		case _ISTACK:			/* stack index register */
			ptr = "PSHS U";
			stack_level += 2;
			byte = last_byte;
			zflag = 0x55;
			break;
		case _TO_TEMP:			/* copy accumulator to temp */
			ptr = byte ? "STB ?temp" : "STD ?temp" ;
			break;
		case _FROM_INDEX:		/* copy index to accumulator */
			ptr = "TFR U,D";
			last_byte = byte = 0;
			zflag = -1;
			break;
		case _COM:				/* complement accumulator */
			if(byte)
				ptr = "COMB";
			else {
				ptr = "COMA\n COMB";
				zflag = -1; }
			break;
		case _NEG:				/* negate accumulator */
			ptr = byte ? "NEGB" : "COMA\n COMB\n ADDD #1";
			break;
		case _NOT:				/* logical complement */
			eflag = -1;
			ptr = "JSR ?not";
			break;
		case _INC:				/* increment accumulator */
			if(isp16(rtype)) {
				ptr = "ADDD #2";
				eflag = -1; }
			else
				ptr = byte ? "INCB" : "ADDD #1";
			break;
		case _DEC:				/* decrement accumulator */
			if(isp16(rtype)) {
				ptr = "SUBD #2";
				eflag = -1; }
			else
				ptr = byte ? "DECB" : "SUBD #1";
			break;
		case _IADD:				/* add acc to index register */
			eflag = -1;
			ptr = "LEAU D,U";
			zflag = 0x55;
			break;
		default:				/* Unknown (error) */
			ptr = "?S?"; }

	if(eflag || !byte)
		expand(rtype);
	else
		last_byte = byte;

	if(zflag != 0x55)
		zero_flag = zflag;

	out_inst(ptr);
}

/*
 * Perform an operation with the accumulator and
 * the specified value;
 */
void accval(unsigned int oper, unsigned int rtype, unsigned int token, unsigned int value, unsigned int type)
{
	char *ptr, *ptr1, byte, rbyte, eflag, zflag;

	byte = rbyte = eflag = zflag = 0;
	ptr1 = 0;

/* values on stack are always words */
	if(token == ON_STACK)
		type &= ~BYTE;

/* determine of length of source & result */
	if((type & (BYTE | POINTER)) == BYTE)
		byte = -1;
	if((rtype & (BYTE | POINTER)) == BYTE)
		rbyte = last_byte;

	switch(oper) {
		case _LOAD:				/* load accumulator */
			ptr = (rbyte = byte) ? "LDB |" : "LDD |";
			last_byte = 0;		/* insure no pre - sign extend */
			break;
		case _STORE:	/* store accumulator */
			ptr = byte ? "STB |" : "STD |";
			break;
		case _ADD:		/* addition */
			if(byte) {
				ptr = "ADDB |";
				if(!rbyte)
					ptr1 = "ADCA #0"; }
			else
				ptr = "ADDD |";
			break;
		case _SUB:		/* subtract */
			if(byte) {
				ptr = "SUBB |";
				if(!rbyte)
					ptr1 = "SBCA #0"; }
			else
				ptr = "SUBD |";
			break;
		case _MULT:		/* multiply */
			zflag = eflag = -1;
			if(token == NUMBER) {
				if(value == 2) { ptr = "LSLB\n ROLA"; break; }
				if(value == 4) { ptr = "LSLB\n ROLA\n LSLB\n ROLA"; break; }
				if(value == 8) { ptr = "LSLB\n ROLA\n LSLB\n ROLA\n LSLB\n ROLA"; break; } }
			ptr = runtime("?mul", type, 0);
			break;
		case _DIV:		/* divide */
			ptr = runtime(((rtype & UNSIGNED) ? "?div" : "?sdiv"),
				type, zflag = eflag = -1);
			break;
		case _MOD:		/* remainder */
			ptr = runtime(((rtype & UNSIGNED) ? "?mod" : "?smod"),
				type, zflag = eflag = -1);
			break;
		case _AND:		/* logical and */
			if(byte) {
				ptr = "ANDB |";
				if(!rbyte)
					ptr1 = "CLRA"; }
			else {
				ptr = "ANDA [\n ANDB ]";
				zflag = -1; }
			break;
		case _OR:		/* logical or */
			if(byte)
				ptr = "ORB |";
			else {
				ptr = "ORA [\n ORB ]";
				zflag = -1; }
			break;
		case _XOR:		/* exclusive or */
			if(byte)
				ptr = "EORB |";
			else {
				ptr = "EORA [\n EORB ]";
				zflag = -1; }
			break;
		case _SHL:		/* shift left */
			ptr = runtime("?shl", type, zflag = eflag = -1);
			break;
		case _SHR:		/* shift right */
			ptr = runtime("?shr", type, zflag = eflag = -1);
			break;
		case _EQ:		/* test for equal */
			eflag = -1;
			/* NOTE: the zero_flag optimisation is NOT applied here.
			 * For != 0, jump_if(FALSE) correctly emits LBEQ (branch when
			 * value is zero = condition is false).  For == 0 the same
			 * LBEQ branches when value IS zero, i.e. when the condition
			 * is TRUE — the opposite of what is wanted.  Always use ?eq
			 * so the result in D is 0/1 and jump_if works correctly. */
			ptr = runtime("?eq", type, 0);
			break;
		case _NE:		/* test for not equal */
			eflag = -1;
			if(token == NUMBER && value == 0) {
				expand(rtype);
				zero_flag = -1;
				return; }
			ptr = runtime("?ne", type, 0);
			break;
		case _LT:		/* test for less than */
			ptr = runtime("?lt", type, eflag = -1);
			break;
		case _LE:		/* test for less or equal to */
			ptr = runtime("?le", type, eflag = -1);
			break;
		case _GT:		/* test for greater than */
			ptr = runtime("?gt", type, eflag = -1);
			break;
		case _GE:		/* test for greater than or equal to */
			ptr = runtime("?ge", type, eflag = -1);
			break;
		case _ULT:		/* unsigned less than */
			ptr = runtime("?ult", type, eflag = -1);
			break;
		case _ULE:		/* unsigned less than or equal to */
			ptr = runtime("?ule", type, eflag = -1);
			break;
		case _UGT:		/* unsigned greater than */
			ptr = runtime("?ugt", type, eflag = -1);
			break;
		case _UGE:		/* unsigned greater than or equal to */
			ptr = runtime("?uge", type, eflag = -1);
			break;
		default:		/* Unknown (error) */
			ptr = "?D? |"; }

/* if necessary, extend acc before executing instruction */
	if(eflag || !rbyte)
		expand(rtype);
	else
		last_byte = rbyte;

/* interpret the output string & insert the operands */
	out_sp();
	while(*ptr) {
		if(*ptr == '|')
			write_oper(token, value, type, 0);
		else if(*ptr == '[')
			write_oper(token, value, type, 1);
		else if(*ptr == ']')
			write_oper(token, value, type, 2);
		else
			out_chr(*ptr);
		++ptr; }
	out_nl();

	zero_flag = zflag;

	if(ptr1) {
		out_inst(ptr1);
		zero_flag = -1; }
}

/*
 * Output a symbol name
 */
void out_symbol(unsigned int s)
{
	if(s_type[s] & STATIC) {		/* Static, output local label */
		out_chr('?');
		out_num(s_index[s]); }
	out_str(s_name[s]);
}

/*
 * Write an instruction to the output file
 */
void out_inst(char *ptr)
{
	put_chr(' ', -1);
	put_str(ptr, -1);
	put_chr('\n', -1);
}

/*
 * Write a string to the output file
 */
void out_str(char *ptr)
{
	put_str(ptr, -1);
}

/*
 * Write a signed number to the output file
 */
void out_num(unsigned int value)
{
	if(value & 0x8000) {
		out_chr('-');
		value = -value; }

	put_num(value, -1);
}

/*
 * Write newline/space characters to the output file.
 */
void out_nl(void) { put_chr('\n', -1); }
void out_sp(void) { put_chr(' ', -1); }

/*
 * Write a character to the output file
 */
void out_chr(char chr)
{
	put_chr(chr, -1);
}
