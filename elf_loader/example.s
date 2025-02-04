section .bss
	char resb 2

section .text
	global _start;
_start:
	mov r13, 3
	mov r14, 4
	add r14, r13
	add r14, 48 ; to ascii	

	mov [char], r14
	mov byte [char+1], 10 ; newline

	mov rax, 1 ; write
	mov rdi, 1 ; std out
	mov rsi, char ; pointer to char buffer
	mov rdx, 2 ; how many bytes to write
	syscall

; Sys_exit
	mov rax, 60
	xor rdi, rdi
	syscall
