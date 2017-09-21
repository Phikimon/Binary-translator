CALL $main
OUT  %0
JMP  $END

;FUNCTION Poschitai_Faktorial
Poschitai_Faktorial:

; Computing value for IF1
PUSH %1 ; En
INR %4 $2
PUSH %4 ; imm

POP %3
POP %2

LGEBIN %2 %3

PUSH %2

POP %2
JE %2 $0 $ELSE1

; RETURN from Poschitai_Faktorial
PUSH %1 ; En

; CALL Poschitai_Faktorial

; Prologue for 'Poschitai_Faktorial' function call
PUSH %0
PUSH %1
PUSH %2
PUSH %3
PUSH %1 ; En
INR %4 $1
PUSH %4 ; imm

POP %3
POP %2

SUBBIN %2 %3

PUSH %2
POP %1
; Prologue end
CALL $Poschitai_Faktorial

; Epilogue for 'Poschitai_Faktorial' function call
; Swap %0 and stack top
POP %2
POP %1
POP %2
POP %3
PUSH %0
MOV %0 %2
; Epilogue end
; CALL_END Poschitai_Faktorial

POP %3
POP %2

MULBIN %2 %3

PUSH %2

POP %0
RET

JMP $IFEND1

ELSE1:

; RETURN from Poschitai_Faktorial
INR %4 $1
PUSH %4 ; imm

POP %0
RET

IFEND1:
; END OF FUNCTION Poschitai_Faktorial

;FUNCTION main
main:

 IN %1 ; Satana

; RETURN from main

; CALL Poschitai_Faktorial

; Prologue for 'Poschitai_Faktorial' function call
PUSH %0
PUSH %1
PUSH %2
PUSH %3
PUSH %1 ; Satana
POP %1
; Prologue end
CALL $Poschitai_Faktorial

; Epilogue for 'Poschitai_Faktorial' function call
; Swap %0 and stack top
POP %2
POP %1
POP %2
POP %3
PUSH %0
MOV %0 %2
; Epilogue end
; CALL_END Poschitai_Faktorial

POP %0
RET
; END OF FUNCTION main


END:
NOP
