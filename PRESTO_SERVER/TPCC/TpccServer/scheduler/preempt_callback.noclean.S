.file	"preempt_callback.c"
.extern preempt

.text
.globl	preempt_callback
.type	preempt_callback, @function

preempt_callback:

	# save used REGISTER: rax
	pushq		%rax

	# save FLAGS
	lahf
	seto		%al
	pushq		%rax

	# NUMBER OF TICKs
	lock incl	tick_count(%rip)

	# preemptable == NOT_PREEMPTABLE ?
	movq		preemptable@gottpoff(%rip), %rax
	movzwl		%fs:(%rax), %eax
	cmpl		$1, %eax
	jne			.L2
	jmp			.L1

.L2:
	# preemptable = NOT_PREEMPTABLE
	movq		preemptable@gottpoff(%rip), %rax
	movw		$1, %fs:(%rax)

	# mode = PLATFORM_MODE
	movq		mode@gottpoff(%rip), %rax
	movw		$1, %fs:(%rax)

	# save REGISTER: rdi
	pushq		%rdi

	# %rdi <-- &running_task->state.context (SINGLE_POOL)
	movq		running_task@gottpoff(%rip), %rdi
	movq		%fs:(%rdi), %rdi
	leaq		272(%rdi), %rdi
	# %rdi <-- &running_task->state->context (TWO_POOLS)
#	movq		272(%rdi), %rdi

	call		_set_jmp

	testl		%eax, %eax
	je			.L3
	jmp			.L4

.L3:
	#
	# SCHEDULE FUNCTION UPDATES THE running_task TLS VARIABLE IF AND ONLY IF A
	# HIGHER PRIORITY TRANSACTION IS FOUND WAITING IN EITHER THE NEW OR THE
	# SUSPEND LIST.
	#
	# TODO:	THIS VERSION OF CALLBACK MUST BE IMPROVED!!! AVOID SET_JMP->LONG_JMP
	#		WHEN NOT NEEDED.
	#
	call		schedule

	# %rdi <-- &running_task->state.context (SINGLE_POOL)
	movq		running_task@gottpoff(%rip), %rdi
	movq		%fs:(%rdi), %rdi
	leaq		272(%rdi), %rdi
	# %rdi <-- &running_task->state->context (TWO_POOLS)
#	movq		272(%rdi), %rdi

	# %rsi <-- A value different from 0
	movq		$1, %rsi

	call		_long_jmp

	# -----no_return----- #

.L4:
	# restore REGISTER: rdi
	popq		%rdi

	# mode = TRANSACTION_MODE
	movq		mode@gottpoff(%rip), %rax
	movw		$0, %fs:(%rax)

	# preemptable = IS_PREEMPTABLE
	movq		preemptable@gottpoff(%rip), %rax
	movw		$0, %fs:(%rax)

.L1:
	# restore FLAGS
	popq		%rax
	addb		$0x7f, %al
	sahf

	# restore REGISTER: rax
	popq		%rax

	retq

.size	preempt_callback, .-preempt_callback
