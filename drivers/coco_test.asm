*
* driver: coco_test
* CoCo character I/O for automated MAME testing.
*
* No screen output. Characters go only to a linear RAM buffer
* polled by the MAME Lua monitor script.
*
* Memory layout:
*   $6FFE-$6FFF  WRIDX: 16-bit write pointer (big-endian)
*                       initialised to BUFBASE by startup code
*   $7000-$77FF  2KB linear output buffer
*
* Lua reads WRIDX to find how many bytes are ready, then reads
* from BUFBASE up to that address.
*
WRIDX   EQU     $0FFE
BUFBASE EQU     $1000
BUFTOP  EQU     $7EFF       * ~28K buffer

* putstr(char *string) - write null-terminated string
putstr  LDX     2,S
?ps1    LDB     ,X+
        BEQ     ?psdone
        TFR     B,A
        BSR     _wrchr
        BRA     ?ps1
?psdone RTS

* putch(char c) - write with LF->CRLF translation
putch   LDD     2,S
        CMPB    #$0A
        BNE     _wrchrB
        LDA     #$0D
        BSR     _wrchr
* putchr(char c) - write raw (stack-call entry)
putchr  LDD     2,S
_wrchrB TFR     B,A
* _wrchr - inner entry: char in A
_wrchr  PSHS    A,X
        LDX     WRIDX       * current write address
        CMPX    #BUFTOP     * buffer full?
        BHI     ?wdone
        STA     ,X+         * store char, advance pointer
        STX     WRIDX       * save updated pointer
?wdone  PULS    A,X
        RTS

* getchr/getch/chkchr/chkch/getstr - stubs, not needed for tests
getchr
getch
chkchr
chkch   CLRA
        CLRB
        RTS
getstr  RTS
