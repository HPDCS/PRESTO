/*
 *	arch/atomic.c
 *
 *	@author:	Emiliano Silvestri
 *	@email:		emisilve86@gmail.com
 */

#include "atomic.h"

inline int P_CAS(volatile void** ptr, void* oldVal, void* newVal) {
	/*
	 * P_CAS implements a compare-and-swap atomic operation on x86-64 for void* data type
	 *
	 * @param
	 * ptr: the address of the void* variable addressing to the real data
	 * oldVal: the old data address we expect to find before swapping
	 * newVal: the new data address to place in *ptr
	 *
	 * @ret
	 * res: 1 if the P_CAS succeeds, 0 otherwise
	 */
	unsigned long res = 0;
	switch (sizeof(*ptr)) {
		case __X86_B:
			__asm__ __volatile__(
				"lock cmpxchgb %1, %2;"
				"lahf;"
				"bt $14, %%ax;"
				"adc %0, %0"
				: "=r"(res)
				: "r"(newVal), "m"(*ptr), "a"(oldVal), "0"(res)
				: "memory"
			);
			break;
		case __X86_W:
			__asm__ __volatile__(
				"lock cmpxchgw %1, %2;"
				"lahf;"
				"bt $14, %%ax;"
				"adc %0, %0"
				: "=r"(res)
				: "r"(newVal), "m"(*ptr), "a"(oldVal), "0"(res)
				: "memory"
			);
			break;
		case __X86_L:
			__asm__ __volatile__(
				"lock cmpxchgl %1, %2;"
				"lahf;"
				"bt $14, %%ax;"
				"adc %0, %0"
				: "=r"(res)
				: "r"(newVal), "m"(*ptr), "a"(oldVal), "0"(res)
				: "memory"
			);
			break;
		case __X86_Q:
			__asm__ __volatile__(
				"lock cmpxchgq %1, %2;"
				"lahf;"
				"bt $14, %%ax;"
				"adc %0, %0"
				: "=r"(res)
				: "r"(newVal), "m"(*ptr), "a"(oldVal), "0"(res)
				: "memory"
			);
			break;
		default:
			break;
	}
	return (int) res;
}

inline int I_CAS(volatile unsigned int *ptr, unsigned int oldVal, unsigned int newVal) {
	/*
	 * I_CAS implements a compare-and-swap atomic operation on x86-64 for integer data type
	 *
	 * @param
	 * ptr: the address of the integer variable
	 * oldVal: the old integer value we expect to find before swapping
	 * newVal: the new integer value to place in *ptr
	 *
	 * @ret
	 * res: 1 if the I_CAS succeeds, 0 otherwise
	 */
    unsigned long res = 0;
    __asm__ __volatile__(
            "lock cmpxchgl %1, %2;"
            "lahf;"
            "bt $14, %%ax;"
            "adc %0, %0"
            : "=r"(res)
            : "r"(newVal), "m"(*ptr), "a"(oldVal), "0"(res)
            : "memory"
    );
    return (int) res;
}

inline void atomic_inc(volatile int* count) {
	__asm__ __volatile__(
		"lock incl %0"
		: "=m" (*count)
		: "m" (*count)
	);
}

inline void atomic_dec(volatile int* count) {
	__asm__ __volatile__(
		"lock decl %0"
		: "=m" (*count)
		: "m" (*count)
	);
}
