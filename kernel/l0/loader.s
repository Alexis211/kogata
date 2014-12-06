[EXTERN kmain]            ; kmain is defined in kmain.c
[GLOBAL loader]           ; making entry point visible to linker
[GLOBAL kernel_pd]   	  ; make kernel page directory visible
[GLOBAL kernel_stack_protector] 		; used to detect kernel stack overflow

; higher-half kernel setup
K_HIGHHALF_ADDR     equ 0xC0000000
K_PAGE_NUMBER       equ (K_HIGHHALF_ADDR >> 22)

; loader stack size
LOADER_STACK_SIZE   equ 0x8000		; 8Kb

; setting up the Multiboot header - see GRUB docs for details
MODULEALIGN         equ 1<<0                   ; align loaded modules on page boundaries
MEMINFO             equ 1<<1                   ; provide memory map
FLAGS               equ MODULEALIGN | MEMINFO  ; this is the Multiboot 'flag' field
MAGIC               equ 0x1BADB002           ; 'magic number' lets bootloader find the header
CHECKSUM            equ -(MAGIC + FLAGS)        ; checksum required

[section .setup]
align 4
multiboot_header:
	dd MAGIC
	dd FLAGS
	dd CHECKSUM

loader:	
	; setup the boot page directory used for higher-half
	mov ecx, kernel_pd
	sub ecx, K_HIGHHALF_ADDR	; access its lower-half address
	mov cr3, ecx

	; Set PSE bit in CR4 to enable 4MB pages.
	mov ecx, cr4
	or ecx, 0x00000010
	mov cr4, ecx

	; Set PG bit in CR0 to enable paging.
	mov ecx, cr0
	or ecx, 0x80000000
	mov cr0, ecx

	; long jump required
	lea ecx, [higherhalf]
	jmp ecx

[section .data]
align 0x1000
kernel_pd:
	; uses 4MB pages
	; identity-maps the first 4Mb of RAM, and also maps them with offset += k_highhalf_addr
	dd 0x00000083
	times (K_PAGE_NUMBER - 1) dd 0
	dd 0x00000083
	times (1024 - K_PAGE_NUMBER - 1) dd 0

[section .text]
higherhalf:		; now we're running in higher half
	; unmap first 4Mb
	mov dword [kernel_pd], 0
	invlpg [0]

	mov esp, stack_top          ; set up the stack

	push eax                    ; pass Multiboot magic number
	add ebx, K_HIGHHALF_ADDR    ; update the MB info structure so that it is in higher half
	push ebx                    ; pass Multiboot info structure

	call  kmain                 ; call kernel proper

hang:
	; halt machine should kernel return
	cli
	hlt
	jmp   hang

[section .bss]
align 0x1000
kernel_stack_protector:
	resb 0x1000			; as soon as we have efficient paging, we WON'T map this page
stack_bottom:
	resb LOADER_STACK_SIZE
stack_top:

; vim: set ts=4 sw=4 tw=0 noet :
