[GLOBAL setjmp]
setjmp:
	; Store general purpose registers
	; (in new stack frame)
	mov [esp+4], eax
	mov [esp+8], ebx
	mov [esp+12], ecx
	mov [esp+16], edx
	mov [esp+20], edi
	mov [esp+24], esi
	mov [esp+28], ebp
	mov [esp+32], esp

	; Store flags
	pushf
	pop eax
	mov [esp+36], eax

	; Store return address
	mov eax, [esp]
	mov [esp+40], eax

	; return 0
	xor eax, eax
	ret


[GLOBAL longjmp]
longjmp:
	; on previous stack, resume return address
	mov eax, [esp+32]
	mov ebx, [esp+40]
	mov [eax], ebx

	; resume flags
	mov eax, [esp+36]
	push eax
	popf

	; load return value in eax
	mov eax, [esp+44]
	; resume geneal purpose registers, except eax/esp
	mov ebx, [esp+8]
	mov ecx, [esp+12]
	mov edx, [esp+16]
	mov edi, [esp+20]
	mov esi, [esp+24]
	mov ebp, [esp+28]

	; resume previous esp
	mov esp, [esp+32]
	; return as if we were the setjmp call
	ret
