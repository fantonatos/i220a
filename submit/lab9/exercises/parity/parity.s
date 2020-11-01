	.text
	.globl get_parity
#edi contains n	
get_parity:

	movl $0, %eax
	test %edi, %edi
	jpe even
	ret
even:
	movl $1, %eax
	ret
