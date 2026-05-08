/*
 * Global definitions for the DDS MICRO-C compiler.
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

#include <stdbool.h>

/*
 * Misc. fixed compiler parameters.
 *
 * When porting to systems with limited memory, you may have to reduce
 * the size of some of the tables in order to make it fit. The most
 * suitable candidates for memory reduction are (in this order):
 *
 *	LITER_POOL, SYMBOL_SIZE, MAX_SYMBOL, DEFINE_POOL, MAX_DEFINE
 */
#define LINE_SIZE	512			/* size of input line */
#define	FILE_SIZE	256			/* maximum size of file name */
#define SYMBOL_SIZE	31			/* # significant chars in symbol name */
#define INCL_DEPTH	10			/* maximum depth of include files */
#define DEF_DEPTH	10			/* maximum depth of define macros */
#define EXPR_DEPTH	20			/* maximum depth of expression stack */
#define LOOP_DEPTH	20			/* maximum depth of nested loops */
#define MAX_ARGS	25			/* maximum # arguments to a function */
#define MAX_SWITCH	50			/* maximum # active switch-case statements */
#define MAX_ERRORS	25			/* # error before termination forced */
#define MAX_DIMS	2000		/* maximum # active array dimensions */
#define MAX_DEFINE	500			/* maximum # define symbols */
#define DEFINE_POOL	8192		/* size of define string space */
#define MAX_TYPEDEF	128			/* maximum # typedef names */
#define MAX_SYMBOL	4000		/* maximum # active symbols */
#define LITER_POOL	65535		/* size of literal string space */

/*
 * Bits found in the "type" entry of symbol table, also
 * used on the expression stack to keep track of element types,
 * and passed to the code generation routines to identify types.
 *
 * The POINTER definition indicates the level of pointer indirection,
 * and should contain all otherwise unused bits, in the least significant
 * (rightmost) bits of the word.
 */
#define REFERENCE	0x8000		/* symbol has been referenced */
#define INITED		0x4000		/* symbol is initialized */
#define ARGUMENT	0x2000		/* symbol is a function argument */
#define EXTERNAL	0x1000		/* external attribute */
#define STATIC		0x0800		/* static attribute */
#define REGISTER	0x0400		/* register attribute */
#define	TVOID		0x0200		/* void attribute */
#define UNSIGNED	0x0100		/* unsigned attribute */
#define BYTE		0x0080		/* 0=16 bit, 1=8 bit */
#define	CONSTANT	0x0040		/* constant attribute */
#define	SYMTYPE		0x0030		/* symbol type (see below) */
#define ARRAY		0x0008		/* symbol is an array */
#define POINTER		0x0007		/* level of pointer indirection */
/* Extended type bits — safe on LP64 Linux where unsigned is 32 bits */
#define LONG_TYPE	0x10000		/* 32-bit storage type (long) */

/*
 * Symbol type designator bits (SYMTYPE field above)
 */
#define	VARIABLE	0x00		/* Symbol is a simple variable */
#define	MEMBER		0x10		/* Symbol is a structure member */
#define	STRUCTURE	0x20		/* Symbol is a structure template */
#define	FUNCGOTO	0x30		/* function(global) or gotolabel(local) */

/*
 * Non-literal tokens identifing value types.
 */
#define NUMBER		100		/* numeric constant */
#define STRING		101		/* literal constant */
#define LABEL		102		/* label address */
#define SYMBOL		103		/* symbol value */
#define IN_ACC		104		/* value in accumulator */
#define IN_TEMP		105		/* value is in temporary register */
#define INDIRECT	106		/* indirect through index register */
#define ON_STACK	107		/* value in on stack */
#define ION_STACK	108		/* indirect through stack top */
#define ISTACK_TOP	109		/* indirect through stack top, leave on */

/*
 * Function compilation state of the parser (0 = No function)
 */
#define	FUNC_ARGS	1		/* Declaring arguments */
#define	FUNC_VARS	2		/* Declaring local variables */
#define	FUNC_CODE	3		/* Outputing code */

/*
 * Register stacking identifiers
 */
#define	STACK_ACC	0x01	/* Stack the accumulator */
#define	STACK_IDX	0x02	/* Stack the index register */

/*
 * No operand operations passed to the
 * code generation routine "accop".
 */
#define _STACK		0		/* place accumulator on stack */
#define _ISTACK		1		/* place index register on stack */
#define _TO_TEMP	2		/* place acc in temporary location */
#define _FROM_INDEX	3		/* get acc from index */
#define _COM		4		/* one's complement acc */
#define _NEG		5		/* two's complement acc */
#define _NOT		6		/* logical complement acc */
#define _INC		7		/* Increment accumulator */
#define _DEC		8		/* decrement accumulator */
#define _IADD		9		/* Add accumulator to index */

/*
 * One operand accumulator operations passed
 * to the code generation routine "accval".
 */
#define _LOAD		0		/* Load register contents */
#define _STORE		1		/* Store the register value */
#define _ADD		2		/* Addition */
#define _SUB		3		/* Subtraction */
#define _MULT		4		/* Multiplication */
#define _DIV		5		/* Division */
#define _MOD		6		/* Modular division */
#define _AND		7		/* Logical AND */
#define _OR		8		/* Logical OR */
#define _XOR		9		/* Exclusive OR */
#define _SHL		10		/* Shift left */
#define _SHR		11		/* Shift right */
#define _EQ		12		/* Test for equals */
#define _NE		13		/* Test for not equals */
#define _LT		14		/* Test for less than */
#define _LE		15		/* Test for less than or equals */
#define _GT		16		/* Test for greater than */
#define _GE		17		/* Test for greater than or equals */
#define _ULT		18		/* Unsigned LT */
#define _ULE		19		/* Unsigned LE */
#define _UGT		20		/* Unsigned GT */
#define _UGE		21		/* Unsigned GE */

extern char s_name[MAX_SYMBOL][SYMBOL_SIZE+1];
extern unsigned int s_type[];
extern unsigned int s_index[];
extern unsigned int s_dindex[];
extern unsigned int dim_pool[];
extern unsigned int global_top;
extern unsigned int local_top;
extern unsigned int function_count;
extern char line;
extern char comment;
extern char fold;
extern char file_name[INCL_DEPTH][FILE_SIZE+1];
extern unsigned int file_depth;

extern char *copy_string(char *dest, char *source);
extern bool equal_string(char const *str1, char const *str2);
extern bool is_num(char chr);
extern bool is_symbol(char chr);
extern bool is_skip(char chr);
extern void compile(void);
extern void inline_asm(void);
extern void statement(unsigned int token);
extern void check_loop(unsigned int stack[]);
extern unsigned int check_switch(void);
extern void test_jump(unsigned int label);
extern bool test_exit(void);
extern void cond_jump(unsigned int term, unsigned int cond, unsigned int label);
extern char *match(char *ptr);
extern char *add_pool(char *string);
extern char *lookup_macro(void);
extern char *parse(void);
extern void unget_token(unsigned int token);
extern bool test_next(unsigned int token);
extern unsigned int get_token(void);
extern unsigned int get_number(unsigned int base, unsigned int digits);
extern void skip_comment(void);
extern int read_special(char delim);
extern int read_char(void);
extern void read_line(void);
extern void test_if(int flag);
extern void skip_cond(void);
extern void expect(unsigned int token);
extern void expect_symbol(void);
extern void get_smname(unsigned int type);
extern void undef_error(void);
extern void symbol_error(char *msg);
extern void symbol_error_warn(char *msg);
extern void text_error(char *msg, char *txt);
extern void syntax_error(void);
extern void index_error(void);
extern void type_error(void);
extern void pointer_error(void);
extern void check_void(unsigned int type);
extern void line_error(char *message);
extern void line_warning(char *message);
extern void severe_error(char *string);
extern unsigned int get_type(unsigned int token, unsigned int type);
extern void define_typedef(void);
extern void declare(unsigned int token, unsigned int type);
extern unsigned int define_structure(void);
extern void define_var(unsigned int type, unsigned int ssize);
extern void define_func(unsigned int type);
extern bool declare_arg(void);
extern void define_symbol(unsigned int type, unsigned int dim_index);
extern void test_redefine(unsigned int type, unsigned int dim_index, char *message);
extern unsigned int lookup_local(void);
extern unsigned int lookup_global(void);
extern unsigned int lookup(void);
extern void special_name(void);
extern void check_type(unsigned int type);
extern void check_func(void);
extern unsigned int size_of_var(unsigned int index);
extern unsigned int get_constant(unsigned int *value);

extern void evaluate(unsigned int term, int xpend);
extern void sub_eval(unsigned int term);
extern unsigned int do_oper(unsigned int token);
extern void push(unsigned int token, unsigned int value, unsigned int type);
extern void pop(unsigned int *token, unsigned int *value, unsigned int *type);
extern void get_value(void);
extern void load_index(unsigned int t, unsigned int v, unsigned int tt);
extern void do_unary(unsigned int oper);
extern void do_binary(unsigned int oper);
extern unsigned int combine(unsigned int type1, unsigned int type2);
extern unsigned int partial_stack(unsigned int token);
extern void stack_register(char rflags);
extern void do_pending(void);
extern bool test_not(void);
extern void load(unsigned int atype, unsigned int token, unsigned int value, unsigned int type);
extern void store(unsigned int atype, unsigned int token, unsigned int value, unsigned int type);

extern char *copy_string(char *dest,char *source);
extern char get_lin(char *);
extern bool f_open(char *);
extern void compile(void);
extern void f_close(void);
extern void put_chr(char,char);
extern void put_num(unsigned int,unsigned int);
extern void put_str(char *,unsigned int);
extern void severe_error(char *string);
extern void terminate(int);

extern char *runtime(char *string, unsigned int type, char order);
extern void write_oper(unsigned int token, unsigned int value, unsigned int type, unsigned int flag);
extern bool isp16(unsigned int type);
extern void do_comment(char *ptr);
extern void do_asm(char *ptr);
extern void release_stack(unsigned int size);
extern void def_module(void);
extern void end_module(void);
extern void def_static(unsigned int symbol,int ssize);
extern void init_static(unsigned int token, unsigned int value, char word);
extern void end_static(void);
extern void def_global(unsigned int symbol, unsigned int size);
extern void def_extern(unsigned int symbol);
extern void def_func(unsigned int symbol, unsigned int size);
extern void end_func(void);
extern void dump_symbol(unsigned int s);
extern void def_label(unsigned int label);
extern void def_literal(char *ptr, unsigned int size);
extern void call(unsigned int token, unsigned int value, unsigned int type, unsigned int clean);
extern void jump(unsigned label, bool ljmp);
extern void jump_if(char cond, unsigned int label, char ljmp);
extern void do_switch(unsigned int label);
extern void index_ptr(unsigned int token, unsigned int value, unsigned int type);
extern void index_adr(unsigned int token, unsigned int value, unsigned int type);
extern void expand(unsigned int type);
extern void narrow_byte(void);
extern void accop(unsigned int oper, unsigned int rtype);
extern void accval(unsigned int oper, unsigned int rtype, unsigned int token, unsigned int value, unsigned int type);
extern void out_symbol(unsigned int s);
extern void out_inst(char *ptr);
extern void out_str(char *ptr);
extern void out_num(unsigned int value);
extern void out_nl(void);
extern void out_sp(void);
extern void out_chr(char chr);
