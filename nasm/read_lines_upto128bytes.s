section .data
	filename db 'lines', 0
	len dq 32
	iterm dd 0xFFFFFFFF
	file_descriptor dd 666
	buffer times 128 db 0
	char db 0xa
	iter dd 0
	counter dd 0
	ascii times 32 db 0
	_chars_printed dq 0

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
	xor r10, r10
	xor r13, r13
	mov r12, [counter]
	call _asciinator
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

; _asciinator requires:
; r10 for iterator, r13 for reverse iterator
; _chars_printed for printing ascii
; r12 should hold the value you'd like to turn into ascii
_asciinator:
	xor rdx, rdx
	mov rax, r12
	mov rbx, 10
	div rbx
	
	mov r12, rax

	add rdx, '0'
	mov [_chars_printed + r10], rdx
	inc r10

	cmp rax, 0
	jne _asciinator	
	
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

