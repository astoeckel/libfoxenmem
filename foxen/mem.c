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

#include <foxen/mem.h>

/******************************************************************************
 * PUBLIC C API                                                               *
 ******************************************************************************/

uint32_t fx_mem_pool_alloc(uint32_t allocated_ptr[], uint32_t *free_idx_ptr,
                           uint32_t *n_allocated_ptr, uint32_t n_available) {
/* Bitmap entry width (BMEW) */
#define FX_BMEW (8U * sizeof(allocated_ptr[0]))
	while (true) {
		/* Load the current free_idx, the index we assume might be free. Then,
		 * try to write the index we should search at with the next iteration.
		 * This allows thread to cooperate to some degree, since threads do
		 * never search at exactly the same free_idx_ (except after a wrap
		 * around).
		 *
		 * There is still some congestion since allocations are stored in a
		 * bitmap, and writing to this bitmap will invalidate the entire
		 * cache line (corresponding to 512 entries for a 64 byte cache line).
		 *
		 * Note that free_idx is only a suggestion as to where we are likely
		 * to find a free entry, it is not a guarantee of any kind that this
		 * is actually true. We will linearly iterate along the slot indices to
		 * find a new entry. */
		uint32_t idx = __atomic_load_n(free_idx_ptr, __ATOMIC_SEQ_CST);
		while (!__atomic_compare_exchange_n(free_idx_ptr, &idx,
		                                    (idx + 1U) % n_available, false,
		                                    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			;

		/* Abort if there is no more space and all slots have been allocated.
		 * Note that we may wrongly abort here if a slot is just in the progress
		 * of being freed, but this is okay. Similarly, we may pass here
		 * although all slots have been allocated and the variable has not yet
		 * been incremented, but this is okay too, since this case will be
		 * catched further below. */
		uint32_t n_allocated =
		    __atomic_load_n(n_allocated_ptr, __ATOMIC_SEQ_CST);
		if (n_allocated >= n_available) {
			return n_available;
		}

		/* Compute the bit-mask and the memory location of the bitmap entry we
		   are trying to modify. */
		const uint32_t mask = (1U << (idx % FX_BMEW));
		uint32_t *allocated_slot_ptr = allocated_ptr + idx / FX_BMEW;

		/* Check whether the entry is in use. If yes, continue with the next
		 * iteration, then try to write the new bitmap entry. If no, try to
		 * write the updated value to the bitmap. If this is successful, update
		 * the number of allocated elements, otherwise continue with the outer
		 * loop and search for a free bitmap entry. */
		uint32_t allocated =
		    __atomic_load_n(allocated_slot_ptr, __ATOMIC_SEQ_CST);
		if (!(allocated & mask) &&
		    __atomic_compare_exchange_n(allocated_slot_ptr, &allocated,
		                                allocated | mask, false,
		                                __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
			/* Writing the new bitmap entry was successful, we need to update
			 * n_allocated by incrementing it by one. There may be a period
			 * where the slot is allocated by setting the corresponding bit in
			 * the bitmap, but the n_allocated counter has not been incremented,
			 * but as explained above, this is fine. */
			while (!__atomic_compare_exchange_n(
			    n_allocated_ptr, &n_allocated, n_allocated + 1U, true,
			    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
				;

			return idx;
		}
	}
#undef FX_BMEW
}

void fx_mem_pool_free(uint32_t idx, uint32_t allocated_ptr[],
                      uint32_t *free_idx_ptr, uint32_t *n_allocated_ptr) {
/* Bitmap entry width (BMEW). */
#define FX_BMEW (8U * sizeof(allocated_ptr[0]))
	/* Load all state variables */
	uint32_t *allocated_slot_ptr = allocated_ptr + idx / FX_BMEW;
	uint32_t allocated = __atomic_load_n(allocated_ptr, __ATOMIC_SEQ_CST);
	uint32_t n_allocated = __atomic_load_n(n_allocated_ptr, __ATOMIC_SEQ_CST);
	uint32_t free_idx = __atomic_load_n(free_idx_ptr, __ATOMIC_SEQ_CST);

	/* Reset the corresponding bit in the bitmap. */
	uint32_t mask = ~(1U << (idx % FX_BMEW));
	while (!__atomic_compare_exchange_n(allocated_slot_ptr, &allocated,
	                                    allocated & mask, true,
	                                    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		;

	/* Decrement the n_allocated counter. Note that the value stored in
	 * n_allocated is temporarily too large (since the slot was already freed),
	 * but this is fine, and at most may temporarily cause fx_mem_alloc() to
	 * fail. This is correct, since fx_mem_free() has not yet finished. */
	while (!__atomic_compare_exchange_n(n_allocated_ptr, &n_allocated,
	                                    n_allocated - 1U, true,
	                                    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		;

	/* Try to update the free_idx, if the given index is smaller than the old
	 * free_idx. This way the allocation will be forced to reuse slots with
	 * small indices and slots at higher addresses have a lower probability of
	 * being used. In situations where this allocator code is used to manage
	 * pages of memory backed by mmap() and "freed" with madvise(), this can
	 * prevent both memory fragmentation and reduce the amount of residual
	 * memory by attempting to keep countiguous memory areas with high addresses
	 * free. */
	while (idx <= free_idx &&
	       !__atomic_compare_exchange_n(free_idx_ptr, &free_idx, idx, true,
	                                    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		;
#undef FX_BMEW
}

