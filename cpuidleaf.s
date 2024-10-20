; This code is for looping over CPU Identification
; 
;
; Now look at the bear
; He's very cute ^^
; ʕ·͡ᴥ͡͡·ʔ


; _leafs will be used for the duration of the program
section .bss
	_leafs resb 8
; For predefined text only
section .data
	leaf_info0 db 'Number of available leafs:', 0x0a
	leaf_info0_2 db 'Vendor ID:', 0x0a

section .text
	global _start;
_start:
_cpuid0:
	push rbp ; function prologue
	push rbx
	push r12
	push r13
	push r14
	push r15		
	mov rbp, rsp

	sub rsp, 16 ; reserve space on stack
	xor eax, eax ; number of leafs you can query
	cpuid
	mov [_leafs], rax ; save leaf count

	mov [rbp - 8], ecx
	mov [rbp - 12], edx
	mov [rbp - 16], ebx
	mov byte [rbp - 4], 0x0A ; New line \n
	mov byte [rbp - 3], 0x00 	

	; Write the number of leafs
	mov rax, 1 ; write
	mov rdi, 1 ; std out
	mov rsi, leaf_info0
	mov rdx, 27
	syscall
	
	mov rdi, [_leafs]
	call _asciinator_loop

; Write the Vendor ID
	mov rax, 1 ; write
	mov rdi, 1 ; std out
	mov rsi, leaf_info0_2
	mov rdx, 11
	syscall	

	mov rax, 1 ; write
	mov rdi, 1 ; std out
	mov r12, rbp
	sub r12, 16
	mov rsi, r12
	mov rdx, 16
	syscall	
	
	jmp _exit;
	; restore registers in the same order
	pop r15
	pop r14
	pop r13
	pop r12
	pop rbx
	pop rbp

_exit: 
	mov rax, 60 ; exit call
	xor edi, edi ; this syscall checks only lower 32 bits
	syscall


; _asciinator_loop:
; rdi should hold the value you'd like to turn into ascii
_asciinator_loop:
	push rbp ; function prologue
	push rbx
	push r12
	push r13
	push r14
	push r15		
	mov rbp, rsp

	sub rsp, 64 ; reserve space on stack
	xor r9, r9	
	mov rbx, 10
	mov rax, rdi
_loop:	
	xor rdx, rdx
	div rbx
	add rdx, '0'
	mov [rbp + r9 - 64], rdx
	inc r9

	cmp rax, 0
	jne _loop		
	cmp r9, 1
	je _write_for_asciinator_loop

	xor r11, r11
	xor r8, r8

	dec r9
	lea r14, [rsp]
	lea r15, [rsp]
	add r15, r9
_reverse_asciinator_loop:
	mov r8b, [r14] 	
	mov r11b, [r15]

	mov [r14], r11b
	mov [r15], r8b

	inc r14
	dec r15	

	cmp r14, r15
	jb _reverse_asciinator_loop
_write_for_asciinator_loop:
	mov byte [rbp - 63 + r9], 0x0A
	mov byte [rbp - 62 + r9], 0x00
	add r9, 3

	lea r14, [rsp]
	mov rax, 1 ; write 
	mov rdi, 1 ; std out
	mov rsi, r14 
	mov rdx, r9
	syscall
	

	add rsp, 64
	; restore registers in the same order
	pop r15
	pop r14
	pop r13
	pop r12
	pop rbx
	pop rbp
	ret

