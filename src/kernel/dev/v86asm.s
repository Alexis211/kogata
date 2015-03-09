
[BITS 32]

[GLOBAL v86_asm_enter_v86]
v86_asm_enter_v86:
	mov eax, [esp + 4]

	xor ebx, ebx
	mov bx, [eax + 20]		; get GS
	push ebx
	mov bx, [eax + 18]		; get FS
	push ebx
	mov bx, [eax + 14]		; get DS
	push ebx
	mov bx, [eax + 16]		; get ES
	push ebx
	mov bx, [eax + 22]		; get SS
	push ebx
	mov bx, [eax + 26]		; get SP
	push ebx
	pushf
	pop ebx
	or ebx, 20200h
	push ebx
	xor ebx, ebx
	mov bx, [eax + 12]		; get CS
	push ebx
	mov bx, [eax + 24]		; get IP
	push ebx

	mov bx, [eax + 2]
	mov cx, [eax + 4]
	mov dx, [eax + 6]
	mov di, [eax + 8]
	mov si, [eax + 10]
	mov ax, [eax]

	iret

