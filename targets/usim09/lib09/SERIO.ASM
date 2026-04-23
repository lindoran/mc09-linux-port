*
* driver: mc6850
* MC6850 ACIA serial I/O driver for Dunfield Micro-C 6809
*
* Register map:
*   $C000+0 = Status (R) / Control (W)
*                               bit {UART_RXRDY_BIT} = RDRF (RX data ready)
*                               bit {UART_TXRDY_BIT} = TDRE (TX data empty)
*   $C000+1   = RX data (R) / TX data (W)
*
* Parameters (substituted by mktarget):
*   UART_BASE    base address of ACIA
*   UART_STATUS  status register offset (typically 0)
*   UART_DATA    data register offset   (typically 1)
*   UART_TXRDY   TX ready bit mask      (typically $02)
*   UART_RXRDY   RX ready bit mask      (typically $01)
*   UART_CRLF    1=translate LF->CRLF on output, 0=raw
*
?uart	EQU	$C000
?ustat	EQU	?uart+0
?udata	EQU	?uart+1
?txrdy	EQU	$02
?rxrdy	EQU	$01
* putstr(char *string) - write null-terminated string
putstr	LDX	2,S
?1	LDB	,X+
	BEQ	?2
	BSR	?putch
	BRA	?1
?2	RTS
* putch(char c) - write char, translating LF->CRLF
putch	LDD	2,S
?putch	CMPB	#$0A		Newline?
	BNE	?putchr
	BSR	?putchr		Write LF
	LDB	#$0D		Then CR
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
