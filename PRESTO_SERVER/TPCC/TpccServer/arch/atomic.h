/*
 *	arch/atomic.h
 *
 *	@author:	Emiliano Silvestri
 *	@email:		emisilve86@gmail.com
 */

#pragma once

#define __X86_B    1
#define __X86_W    2
#define __X86_L    4
#define __X86_Q    8

/* Prototype for the functions declared in "atomic.c" */
inline int		P_CAS(volatile void**, void*, void*);
inline int		I_CAS(volatile unsigned int*, unsigned int, unsigned int);
inline void		atomic_inc(volatile int*);
inline void		atomic_dec(volatile int*);
