/*
 *  libfoxenmen -- Utilities for heap-free memory management
 *  Copyright (C) 2018  Andreas Stöckel
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file mem.h
 *
 * Utility functions for aligning pointers, computing pointer offsets, and the
 * size of an object in memory.
 *
 * @author Andreas Stöckel
 */

#ifndef FOXEN_MEM_H
#define FOXEN_MEM_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * Memory alignment for pointers internally used by Stanchion. Aligning memory
 * and telling the compiler about it allows the compiler to perform better
 * optimization. Furthermore, some platforms (WASM) do not allow unaligned
 * memory access.
 */
#define FX_ALIGN 16

/**
 * Macro telling the compiler that P is aligned with the alignment defined
 * above.
 */
#define FX_ASSUME_ALIGNED(P) P
#ifdef __GNUC__
#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || (__GNUC__ > 4)
#undef FX_ASSUME_ALIGNED
#define FX_ASSUME_ALIGNED(P) (__builtin_assume_aligned(P, FX_ALIGN))
#endif /* (__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || (__GNUC__ > 4) */
#endif /* __GNUC__ */

/**
 * Forces a pointer to have the alignment defined above.
 */
#define FX_ALIGN_ADDR(P) \
	(FX_ASSUME_ALIGNED(  \
	    (void *)(((uintptr_t)(P) + FX_ALIGN - 1) & (~(FX_ALIGN - 1)))))

/**
 * Call this first in a chain of fx_mem_update_size() calls. It will make sure
 * that there is enough space to align the datastructure whenever the user
 * provides a non-aligned target memory pointer.
 *
 * @param size is a pointer at a variable that holds the size of the object
 * that we're describing. This function initializes this value to FX_ALIGN - 1.
 * @return Always returns true to facilitate chaining with other fx_mem_*_size()
 * functions.
 */
static inline bool fx_mem_init_size(uint32_t *size) {
	*size = FX_ALIGN;
	return true;
}

/**
 * Function used to compute the total size of a datastructure consisting of
 * multiple substructures. Calling this function updates the size of the outer
 * datastructure by adding a substructure of size n_bytes. Assumes that the
 * beginning of the substructure must be aligned.
 *
 * @param size is a pointer at the variable holding the size of the
 * datastructure. This must always be a multiple of FX_ALIGN.
 * @param n_bytes size of the sub-structure that should be added.
 * @return zero if there was an overflow, one otherwise.
 */
static inline bool fx_mem_update_size(uint32_t *size, uint32_t n_bytes) {
	const uint32_t new_size =
	    ((*size + n_bytes + FX_ALIGN - 1) & (~(FX_ALIGN - 1)));
	if (new_size < *size) {
		return false; /* error, there has been an overflow */
	}
	*size = new_size;
	return true; /* success */
}

/**
 * Computes the aligned pointer pointing at the substructure of the given size.
 *
 * @param mem pointer at the variable holding the pointer at the current
 * pointer. The pointer is advanced by the given size after the return value is
 * computed.
 * @param size is the size of the substructure for which the pointer should be
 * returned.
 * @return an aligned pointer pointing at the beginning of the substructure.
 */
static inline void *fx_mem_align(void **mem, uint32_t size) {
	void *res = FX_ALIGN_ADDR(*mem);
	*mem = (void *)((uintptr_t)res + size);
	return res;
}

/**
 * Fills the given memory region with zeros. In contrast to memset(mem, 0, size)
 * this assumes that the pointer is aligned at the FX_ALIGN boundary and that
 * we can write multiples of FX_ALIGN bytes at once. This is potentially
 * dangerous, so do not use this function if you don't know exactly what you're
 * doing.
 *
 * @param mem is a pointer at the memory region that should be zeroed out. This
 * pointer is assumed to be aligned.
 * @param size is the size of the memory region that should be zeroed in bytes.
 * This value is effectively rounded up to a multiple of FX_ALIGN
 */
static inline void fx_zero_aligned_mem(void *mem, uint32_t size) {
	assert((((uintptr_t)mem) & (FX_ALIGN - 1)) == 0); /* mem must be aligned */
	mem = FX_ASSUME_ALIGNED(mem);
	for (uint32_t i = 0; i < (size + FX_ALIGN - 1) / FX_ALIGN; i++) {
		((uint64_t*)mem)[2 * i + 0] = 0; /* If we're lucky, this loop is */
		((uint64_t*)mem)[2 * i + 1] = 0; /* unrolled and vectorised. */
	}
}

/**
 * Macro that fills the structure pointed at by P with zeros. See
 * fx_zero_aligned_mem() regarding potential dangers.
 */
#define FX_ZERO_ALIGNED_MEM(P) do { \
	fx_mem_zero(P, sizeof(*(P))); \
} while(0)

#endif /* FOXEN_MEM_H */
