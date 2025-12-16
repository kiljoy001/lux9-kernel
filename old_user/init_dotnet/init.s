.section .note.GNU-stack,"",@progbits
.text
.globl method_100663301
method_100663301:
	pushq %rbp
	movq %rsp, %rbp
	movl $1, %edi
	callq clr_string_from_literal
	callq method_100663300
	movl $37, %edi
	callq clr_string_from_literal
	callq method_100663300
	movl $97, %edi
	callq clr_string_from_literal
	callq method_100663300
	movl $143, %edi
	callq clr_string_from_literal
	callq method_100663300
	xorl %eax, %eax
	leave
	ret
/* end function method_100663301 */

.section .note.GNU-stack,"",@progbits
