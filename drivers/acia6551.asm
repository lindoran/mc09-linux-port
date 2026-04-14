*
* driver: acia6551
* Rockwell 6551 ACIA serial I/O driver for Dunfield Micro-C 6809
*
* Register map (6551):
*   {UART_BASE}+0 = RX data (R) / TX data (W)
*   {UART_BASE}+1 = Status (R) / Programmed Reset (W)
*                   bit 3 = RDRF (RX data ready)
*                   bit 4 = TDRE (TX data empty)
*   {UART_BASE}+2 = Command register
*   {UART_BASE}+3 = Control register
*
* Note: 6551 default register layout differs from 6850.
* Status is at offset 1, data at offset 0.
* Default TX ready bit = $10, RX ready bit = $08.
*
?uart	EQU	{UART_BASE}
?ustat	EQU	?uart+{UART_STATUS}
?udata	EQU	?uart+{UART_DATA}
?txrdy	EQU	{UART_TXRDY}
?rxrdy	EQU	{UART_RXRDY}
* putstr(char *string) - write null-terminated string
putstr	LDX	2,S
?1	LDB	,X+
	BEQ	?2
	BSR	?putch
	BRA	?1
?2	RTS
* putch(char c) - write char, translating LF->CRLF
putch	LDD	2,S
?putch	CMPB	#$0A
{CRLF_YES}	BNE	?putchr
{CRLF_YES}	BSR	?putchr
{CRLF_YES}	LDB	#$0D
{CRLF_NO}	BEQ	?putchr
	BRA	?putchr
* putchr(char c) - write char raw
putchr	LDD	2,S
?putchr	LDA	?ustat
	BITA	#?txrdy
	BEQ	?putchr
	STB	?udata
?3	RTS
* chkchr() - non-blocking: return char or -1
chkchr	LDA	?ustat
	BITA	#?rxrdy
	BNE	getchr
	LDD	#-1
	RTS
* chkch() - non-blocking: return 0 or RDRF bit
chkch	LDA	?ustat
	CLRB
	ANDA	#?rxrdy
	RTS
* getch() - read char, translating CR->LF
getch	BSR	getchr
	CMPB	#$0D
	BNE	?4
	LDB	#$0A
?4	RTS
* getchr() - read char raw
getchr	LDA	?ustat
	BITA	#?rxrdy
	BEQ	getchr
	LDB	?udata
	CLRA
	RTS
* getstr(buf, len) - read line with backspace editing
getstr	LDU	4,S
	LDX	#0
?5	BSR	getch
	CMPB	#$7F
	BEQ	?6
	CMPB	#$08
	BEQ	?6
	CMPB	#$0A
	BEQ	?7
	CMPX	2,S
	BHS	?5
	STB	,U+
	LEAX	1,X
	BSR	?putchr
	BRA	?5
?6	CMPX	#0
	BEQ	?5
	LDB	#$08
	BSR	?putchr
	LDB	#' '
	BSR	?putchr
	LDB	#$08
	BSR	?putchr
	LEAX	-1,X
	LEAU	-1,U
	BRA	?5
?7	CLR	,U
	BSR	?putch
	TFR	X,D
	RTS
