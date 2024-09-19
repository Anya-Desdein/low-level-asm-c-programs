section .bss
	cpudata resb 12 ; reserve 12 bytes

section .data
	counter dq 17

section .text
	global _start;
_start:
	mov eax, 0 ; see how many leafs can I query for
	cpuid
	inc eax
	mov [counter], eax 
_loop:		
	mov rax, [counter]
	dec rax
	mov [counter], rax
	cpuid
	mov [cpudata], rbx
	mov [cpudata + 4], rcx
	mov [cpudata + 8], rdx
	mov rcx, 1
	mov rax, [counter]
	cmp rax, rcx
	jg _loop
	

_exit: 
	mov rax, 60 ; exit call
	xor rdi, rdi
	syscall
