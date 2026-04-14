*
* driver: none
* Stub I/O driver - no serial hardware
*
* All functions return immediately without doing anything.
* Use for bare-metal bring-up, or targets where you provide
* your own I/O by linking additional object modules.
*
* putstr, putch, putchr - do nothing, return immediately
putstr
putch
putchr
?putchr	RTS
* chkchr() - always returns -1 (no char available)
chkchr	LDD	#-1
	RTS
* chkch() - always returns 0 (no char available)
chkch	CLRA
	CLRB
	RTS
* getch, getchr - spin forever (no input possible)
getch
getchr	BRA	getchr
* getstr - return empty string immediately
getstr	LDU	4,S
	CLR	,U
	CLRA
	CLRB
	RTS
