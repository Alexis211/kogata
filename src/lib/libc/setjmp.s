[GLOBAL setjmp]
setjmp:
	; get return address
	mov edx, [esp]
	; get address of jmpbuf structure
	mov ecx, [esp+4]

	; Store general purpose registers
	mov [ecx], ebx
	mov [ecx+4], edx
	mov [ecx+8], ebp
	mov [ecx+12], esp
	mov [ecx+16], esi
	mov [ecx+20], edi
	mov [ecx+24], eax

	; return 0
	xor eax, eax
	ret


[GLOBAL longjmp]
longjmp:
	; get address of jmpbuf structure
	mov ecx, [esp+4]
	; get retun value
	mov eax, [esp+8]

	; load general purpose registers
	; (edx contains the return address)
	mov ebx, [ecx]
	mov edx, [ecx+4]
	mov ebp, [ecx+8]
	mov esp, [ecx+12]
	mov esi, [ecx+16]
	mov edi, [ecx+20]

	; make sure return value is nonzero
	test eax, eax
	jnz _doret
	inc eax
_doret:
	; store back return address
	mov [esp], edx
	ret

