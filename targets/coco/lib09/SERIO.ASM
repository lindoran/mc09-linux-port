*
* driver: coco_rom
* CoCo 1/2/3 character I/O via BASIC ROM vectors
*
* The CoCo BASIC ROM exposes character I/O through a jump vector table
* at $A000. Programs call these vectors rather than the ROM directly,
* so the routines work across different ROM revisions.
*
* Vector table (all are 3-byte JMP instructions):
*   $A000 = POLCAT - poll keyboard, return char in A (0 if no key)
*   $A002 = CHROUT - output char in A to console
*
* Calling convention:
*   CHROUT: char to output in A register
*   POLCAT: returns char in A, Z flag set if no key pressed
*
* These vectors are patched by BASIC at startup to point to the actual
* ROM routines. Using the vectors rather than direct addresses ensures
* compatibility across CoCo 1, 2, and 3.
*
* No parameters needed - vector addresses are fixed in CoCo hardware.
*
* putstr(char *string) - write null-terminated string
putstr	LDX	2,S
?1	LDB	,X+
	BEQ	?2
	TFR	B,A
	BSR	?putch
	BRA	?1
?2	RTS
* putch(char c) - write char, translating LF->CRLF
putch	LDD	2,S
?putch	CMPB	#$0A		Newline?
	BNE	?putchr
	LDA	#$0D		Write CR first
	JSR	[$A002]		CHROUT vector
* putchr(char c) - write char raw
putchr	LDD	2,S
?putchr	TFR	B,A		CoCo CHROUT takes char in A
	JSR	[$A002]		CHROUT vector
?3	RTS
* chkchr() - non-blocking: return char or -1
* Uses POLCAT ($A000) - returns 0 in A if no key, char otherwise
chkchr	JSR	[$A000]		POLCAT vector
	BEQ	?nochr		Z set = no key
	TFR	A,B
	CLRA
	RTS
?nochr	LDD	#-1
	RTS
* chkch() - non-blocking: return 0 or non-zero
chkch	JSR	[$A000]
	TFR	A,B
	CLRA
	RTS
* getch() - blocking read, translating CR->LF
getch	BSR	getchr
	CMPB	#$0D
	BNE	?4
	LDB	#$0A
?4	RTS
* getchr() - blocking read raw
getchr	JSR	[$A000]		Poll
	BEQ	getchr		No key, keep polling
	TFR	A,B
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
	TFR	B,A
	JSR	[$A002]		Echo character
	BRA	?5
?6	CMPX	#0
	BEQ	?5
	LDA	#$08		Backspace
	JSR	[$A002]
	LDA	#' '
	JSR	[$A002]
	LDA	#$08
	JSR	[$A002]
	LEAX	-1,X
	LEAU	-1,U
	BRA	?5
?7	CLR	,U
	LDA	#$0D		Echo CR on entry
	JSR	[$A002]
	TFR	X,D
	RTS
