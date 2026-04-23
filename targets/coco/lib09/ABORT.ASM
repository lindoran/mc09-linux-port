* Terminate execution with an error message
abort	LEAS	2,S		Remove return address
	JSR	putstr		Display string
	JMP	exit		And terminate
$EX:putstr
*50000 8/4/2026
