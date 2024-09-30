; This code is for looping over CPU Identification
; 
;
; Now look at the bear
; He's very cute ^^
; ʕ·͡ᴥ͡͡·ʔ <---------- This wasn't copy pasted...


section .bss
	cpudata resb 14 ; reserve 12 bytes for cpuid data
	; and 2 bytes for new line and null terminator
	_helper resb 128
	_buffer resb 64

section .data
	leaf_info0 db 'Number of available leafs:', 0x0a
	leaf_info0_2 db 'Vendor ID:', 0x0a
	_chars_printed dq 0

section .text
	global _start;
_start:
	xor eax, eax ; number of leafs you can query
	; int3 ; this is sigtrap, it's for debug
	cpuid
	mov r12d,         eax ; save the counter
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
	
	xor r10, r10
	xor r13, r13
	call _asciinator_loop

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
	
	jmp _exit;
_exit: 
	mov rax, 60 ; exit call
	xor edi, edi ; this syscall checks only lower 32 bits
	syscall

; _asciinator_loop requires:
; r10 for iterator, r13 for reverse iterator
; _chars_printed for printing ascii
; r12 should hold the value you'd like to turn into ascii
_asciinator_loop:
	xor rdx, rdx
	mov rax, r12
	mov rbx, 10
	div rbx
	
	mov r12, rax

	add rdx, '0'
	mov [_chars_printed + r10], rdx
	inc r10

	cmp rax, 0
	jne _asciinator_loop		
	
	mov r9, r10
	xor r11, r11
	xor r8, r8	
_reverse_asciinator_loop:
	mov r14, _chars_printed
	mov r15, r14
	add r14, r13
	add r15, r10
	dec r15
	
	mov r11b, [r15]
	mov r8b, [r14] 	
	mov [r15], r8b
	mov [r14], r11b
	dec r10	
	inc r13

	cmp r10, r13
	ja _reverse_asciinator_loop

_write_for_asciinator_loop:
	inc r9
	mov byte [_chars_printed + r9], 0x0A
	inc r9
	mov byte [_chars_printed + r9], 0x00


	mov rax, 1 ; write 
	mov rdi, 1 ; std out
	mov rsi, _chars_printed
	mov rdx, r9
	syscall
	ret

