/*
 * Prototypes for functions in 6809cg.c
 *
 */
 
#ifndef I_9249949C_CA9C_5746_A00B_A3D09E5E31D7
#define I_9249949C_CA9C_5746_A00B_A3D09E5E31D7

extern char *runtime(char *string,unsigned int type,char order);
extern void write_oper(unsigned int token,unsigned int value,unsigned int type,unsigned int flag);
extern bool isp16(unsigned int type);
extern void do_comment(char *ptr);
extern void do_asm(char *ptr);
extern void release_stack(unsigned int size);
extern void def_module(void);
extern void end_module(void);
extern void def_static(unsigned int symbol,int ssize);
extern void init_static(unsigned int token,unsigned int value,char word);
extern void end_static(void);
extern void def_global(unsigned int symbol,unsigned int size);
extern void def_extern(unsigned int symbol);
extern void def_func(unsigned int symbol, unsigned int size);
extern void end_func(void);
extern void dump_symbol(unsigned int s);
extern void def_label(unsigned int label);
extern void def_literal(char *ptr, unsigned int size);
extern void call(unsigned int token, unsigned int value, unsigned int type, unsigned int clean);
extern void jump(unsigned int label, bool ljmp);
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

#endif
