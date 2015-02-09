;************************************************************************************

%macro COMMONSTUB 1
[EXTERN idt_%1Handler]
%1_common_stub:

	pusha                    ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

	mov ax, ds               ; Lower 16-bits of eax = ds.
	push eax                 ; save the data segment descriptor

	mov ax, 0x10  ; load the kernel data segment descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	; pass the register data structure as a pointer to the function
	; (passing it directly results in GCC trashing the data when doing optimisations)
	mov eax, esp
	push eax
	call idt_%1Handler
	add esp, 4

	pop eax        ; reload the original data segment descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	popa                     ; Pops edi,esi,ebp...
	add esp, 8     ; Cleans up the pushed error code and pushed ISR number
	iret
%endmacro

COMMONSTUB ex
COMMONSTUB irq
COMMONSTUB syscall

;************************************************************************************

%macro EX_NOERRCODE 1  ; define a macro, taking one parameter
	[GLOBAL isr%1]        ; %1 accesses the first parameter.
	isr%1:
		push byte 0
		push byte %1
		jmp ex_common_stub
%endmacro

%macro EX_ERRCODE 1
	[GLOBAL isr%1]
	isr%1:
		push byte %1
		jmp ex_common_stub
%endmacro

%macro IRQ 2
	[GLOBAL irq%1]
	irq%1:
		push byte %1	;push irq number
		push byte %2	;push int number
		jmp irq_common_stub
%endmacro

%macro SYSCALL 1
	[GLOBAL syscall%1]
	syscall%1:
		cli
		push byte 0
		push byte %1
		jmp syscall_common_stub
%endmacro

EX_NOERRCODE 0
EX_NOERRCODE 1
EX_NOERRCODE 2
EX_NOERRCODE 3
EX_NOERRCODE 4
EX_NOERRCODE 5
EX_NOERRCODE 6
EX_NOERRCODE 7
EX_ERRCODE 8
EX_NOERRCODE 9
EX_ERRCODE 10
EX_ERRCODE 11
EX_ERRCODE 12
EX_ERRCODE 13
EX_ERRCODE 14
EX_NOERRCODE 15
EX_NOERRCODE 16
EX_NOERRCODE 17
EX_NOERRCODE 18
EX_NOERRCODE 19
EX_NOERRCODE 20
EX_NOERRCODE 21
EX_NOERRCODE 22
EX_NOERRCODE 23
EX_NOERRCODE 24
EX_NOERRCODE 25
EX_NOERRCODE 26
EX_NOERRCODE 27
EX_NOERRCODE 28
EX_NOERRCODE 29
EX_NOERRCODE 30
EX_NOERRCODE 31

IRQ	0,	32
IRQ	1,	33
IRQ	2,	34
IRQ	3,	35
IRQ	4,	36
IRQ	5,	37
IRQ	6,	38
IRQ	7,	39
IRQ	8,	40
IRQ	9,	41
IRQ	10,	42
IRQ	11,	43
IRQ	12,	44
IRQ	13,	45
IRQ	14,	46
IRQ	15,	47

SYSCALL 64

; vim: set ts=4 sw=4 tw=0 noet :
