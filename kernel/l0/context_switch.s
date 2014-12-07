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

	mov eax, [esp+44]		; get address of saved_context structure
	mov [eax], esp			; save esp
	mov dword [eax+4], resume_saved_context		; save eip

	mov esp, kernel_stack_top
	jmp run_scheduler

resume_saved_context:
	pop eax
	mov cr3, eax

	popa
	popf
	ret

[GLOBAL irq0_save_context_and_enter_scheduler]
; meant to be called on IRQ0
; general registers already saved by IRQ handler stub
; flags already saved by interruption and interruptions disabled
; only saves CR3
irq0_save_context_and_enter_scheduler:
	mov eax, cr3
	push eax

	mov eax, [esp+8]		; get address of saved_context structure
	mov [eax], esp			; save esp
	mov dword [eax+4], resume_saved_irq0_context		; save eip

	mov esp, kernel_stack_top
	jmp run_scheduler

resume_saved_irq0_context:
	pop eax
	mov cr3, eax
	ret


[GLOBAL resume_context]
resume_context:
	mov eax, [esp+4]		; get address of saved context
	mov esp, [eax]			; resume esp
	mov ecx, [eax+4]		; jump to specified eip
	jmp ecx

; vim: set ts=4 sw=4 tw=0 noet :
