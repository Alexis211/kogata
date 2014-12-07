[EXTERN kernel_stack_top]
[EXTERN run_scheduler]

[GLOBAL save_context_and_enter_scheduler]
; void save_context_and_enter_scheduler(struct saved_context *ctx);
save_context_and_enter_scheduler:
	pushf
	cli
	pusha					; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

	mov eax, cr3
	push eax

	mov ax, ds              ; Lower 16-bits of eax = ds.
	push eax                ; save the data segment descriptor
	
	mov eax, [esp+48]		; get address of saved_context structure
	mov [eax], esp			; save esp
	mov dword [eax+4], resume_saved_context		; save eip

	mov esp, kernel_stack_top
	jmp run_scheduler

resume_saved_context:
	pop eax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	pop eax
	mov cr3, eax

	popa
	popf
	ret


[GLOBAL resume_context]
resume_context:
	mov eax, [esp+4]		; get address of saved context
	mov esp, [eax]			; resume esp
	mov ecx, [eax+4]		; jump to specified eip
	jmp ecx

; vim: set ts=4 sw=4 tw=0 noet :
