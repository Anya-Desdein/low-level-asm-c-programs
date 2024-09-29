; This code is for looping over CPU Identification
; 
;
; Now look at the bear
; He's very cute ^^
; ʕ·͡ᴥ͡͡·ʔ <---------- This wasn't copy pasted...


section .bss
	cpudata resb 14 ; reserve 12 bytes for cpuid data
	; and 2 bytes for new line and null terminator
	counter resb 1 ; reserve one byte for iterator
	_helper resb 128
	_buffer resb 64

section .data
	leaf_info0 db 'Number of available leafs:', 0x0a
	leaf_info0_2 db 'Vendor ID:', 0x0a
	_chars_printed dq 0
	_iter dq 0

section .text
	global _start;
_start:
	xor eax, eax ; number of leafs you can query
	cpuid
	mov [counter],    al ; save the counter
	mov [cpudata],    ebx
	mov [cpudata+4],  edx
	mov [cpudata+8],  ecx
	mov byte [cpudata + 12], 0x0A ; New line \n
	mov byte [cpudata + 13], 0x00 	

; Write the number of leafs
	mov rax, 1 ; write
	mov rdi, 1 ; std out
	mov rsi, leaf_info0
	mov rdx, 27
	syscall	

_loop:
	xor rdx, rdx
	mov rax, [counter]
	mov rbx, 10
	div rbx
	
	mov [counter], al

	add rdx, '0'
	mov r10, [_iter]
	mov [_chars_printed + r10], rdx
	inc r10
	mov [_iter], r10

	cmp rax, 0
	jne _loop	
	
	mov byte [_chars_printed + r10], 0x0A
	inc r10
	mov byte [_chars_printed + r10], 0x00
		
	mov rax, 1 ; write 
	mov rdi, 1 ; std out
	mov rsi, _chars_printed
	mov rdx, r10
	syscall

; Write the Vendor ID
	mov rax, 1 ; write
	mov rdi, 1 ; std out
	mov rsi, leaf_info0_2
	mov rdx, 11
	syscall	

	mov rax, 1 ; write
	mov rdi, 1 ; std out
	mov rsi, cpudata
	mov rdx, 14
	syscall	


	
	jmp _exit; temp jump to exit 

	mov [counter], al
	cpuid
	mov [cpudata], ebx
	mov [cpudata + 4], edx
	mov [cpudata + 8], ecx
	
	call _write

	mov ecx, 0
	mov al, [counter]
	cmp rax, rcx

_exit: 
	mov rax, 60 ; exit call
	xor edi, edi ; this syscall checks only lower 32 bits
	syscall

_write:
	mov byte [cpudata + 12], 0x0A ; New line \n
	mov byte [cpudata + 13], 0x00 
	mov rax, 1 ; write 
	mov rdi, 1
	mov rsi, cpudata
	mov rdx, 14
	syscall
	ret


