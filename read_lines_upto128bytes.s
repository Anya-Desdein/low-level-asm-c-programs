section .data
	filename db 'lines', 0
	len dq 32
	iterm dd 0xFFFFFFFF
	file_descriptor dd 666
	buffer times 128 db 0
	char db 0xa
	iter dd 0
	counter dd 0
	tomod dd 10
	ascii times 32 db 0
	iter_str db 31

section .text
	global _start;

_start:

	call open
	mov [file_descriptor], rax
	;call print
	call read

_loop:

	mov eax, buffer
	mov ebx, [iter]
	mov cl, [eax + ebx] 	
	
	mov dl, [char]	
	cmp cl, dl
	jne skip_incr_newl
	call incr_newl

skip_incr_newl:
	call incr 	
	mov ebx, [iter]
	cmp ebx, [iterm]
	jae _endmepls
	
	jmp _loop	
	
_endmepls:	
	jmp asciinator
_end:	
	mov rsi, ascii
	call print
	mov rax, 60
	xor rdi, rdi
	syscall

open:
	mov rax, 257
	mov rdi, -100
	lea rsi, [filename]
	mov rdx, 0 	
	mov r10, 0
	syscall	
	ret

print:
	mov rax, 1
	mov rdi, 1
	mov rdx, [len]
	syscall
	ret

read:
	mov rax, 0
	mov rdi, [file_descriptor]
	mov rsi, buffer 
	mov rdx, 128
	syscall
	mov [iterm], eax
	ret	

incr:	
	mov r12d, [iter]
	inc r12d
	mov [iter], r12d
	ret

incr_newl:	
	mov r12d, [counter]
	inc r12d
	mov [counter], r12d
	ret

asciinator:
	mov eax, [counter]
	mov r13d, [tomod] ; divisor, lower 32 bits
	xor edx, edx ; divisor, upper 32 bits
	div r13d
	mov [counter], eax

	add edx, '0'
	
	mov dil, [iter_str]
	movzx rdi, dil
	mov byte [rdi + ascii], dl	
	sub dword [iter_str], 1

	cmp eax, 0
	jne asciinator
	jmp _end
