.bss
	char: .zero 2

.text
	.globl _start

_start:
	mov $3, %r13
	mov $4, %r14
	addq %r13, %r14
	addq $48, %r14 # to ascii	

	movb %r14b, char(%rip)
	movb $10, char+1(%rip) # newline

	mov $1, %rax # write
	mov $1, %rdi # std out
	lea char(%rip), %rsi # pointer to char buffer
	
	mov $1, %rdx # how many bytes to write
	syscall

# Sys_exit
	movq $60, %rax
	xorq %rdi, %rdi
	syscall
