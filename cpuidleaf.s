; This code is for looping over CPU Identification
; 
;
; Now look at the bear
; He's very cute ^^
; ʕ·͡ᴥ͡͡·ʔ <---------- This wasn't copy pasted...


section .bss
	cpudata resb 12 ; reserve 12 bytes for cpuid data
	counter resb 1 ; reserve one byte for iterator

section .text
	global _start;
_start:
	xor eax, eax ; see how many leafs can I query for
	cpuid
	inc eax
	mov [counter], al 
_loop:		
	movzx rax, al
	mov al, [counter]
	dec al
	mov [counter], al
	cpuid
	mov [cpudata], ebx
	mov [cpudata + 4], ecx
	mov [cpudata + 8], edx
	mov ecx, 1
	mov al, [counter]
	cmp rax, rcx
	jg _loop
	

_exit: 
	mov rax, 60 ; exit call
	xor edi, edi ; this syscall checks only lower 32 bits
	syscall
