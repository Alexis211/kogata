[EXTERN kernel_stack_top]
[EXTERN run_scheduler]

[GLOBAL save_context_and_enter_scheduler]
; void save_context_and_enter_scheduler(struct saved_context *ctx);
save_context_and_enter_scheduler:
	push ebp				; save stack frame for debugging
	mov ebp, esp
	pushf					; push flags
	cli
	pusha					; push general registers
	mov eax, cr3			; push CR3
	push eax

	mov eax, [ebp+8]		; get address of saved_context structure
	mov [eax], esp			; save esp
	mov dword [eax+4], resume_saved_context		; save eip

	mov esp, kernel_stack_top
	jmp run_scheduler

resume_saved_context:
	pop eax			; restore CR3
	mov cr3, eax
	popa			; restore general registers
	popf			; restore flags
	pop ebp
	ret


[GLOBAL resume_context]
resume_context:
	push ebp
	mov ebp, esp
	mov eax, [ebp+8]		; get address of saved context
	mov esp, [eax]			; resume esp
	mov ecx, [eax+4]		; jump to specified eip
	jmp ecx

; vim: set ts=4 sw=4 tw=0 noet :
