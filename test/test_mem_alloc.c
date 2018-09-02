/*
 *  libfoxenmem -- Utilities for heap-free memory management
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

#include <pthread.h>

#include <foxen/mem.h>
#include <foxen/unittest.h>

/******************************************************************************
 * UNIT TESTS                                                                 *
 ******************************************************************************/

#define n_available ((1U << 16U) - 17U)
#define n_bitmap_entries ((n_available + 31U) / 32U)
static uint32_t allocated[n_bitmap_entries] __attribute__((aligned(64)));
static uint32_t slots[n_available] __attribute__((aligned(64)));
static uint8_t slot_acquired[n_available] __attribute__((aligned(64)));
static uint32_t free_idx __attribute__((aligned(64)));
static uint32_t n_allocated __attribute__((aligned(64)));

#define ALLOC fx_mem_pool_alloc(allocated, &free_idx, &n_allocated, n_available)
#define FREE(i) fx_mem_pool_free(i, allocated, &free_idx, &n_allocated)

static void _test_reset() {
	free_idx = 0U;
	n_allocated = 0U;
	for (uint32_t i = 0U; i < n_bitmap_entries; i++) {
		allocated[i] = 0U;
	}
	for (uint32_t i = 0U; i < n_available; i++) {
		slots[i] = 0U;
		slot_acquired[i] = 0U;
	}
}

static void test_mem_alloc_free_simple(void) {
	_test_reset();

	/* Calling fx_mem_alloc repeatedly should just iterate from zero to
	   n_available - 1 for the first n_available calls */
	for (uint32_t i = 0U; i < n_available; i++) {
		uint32_t idx = ALLOC;
		EXPECT_EQ(i, idx);
		EXPECT_EQ(i + 1U, n_allocated);
	}

	for (uint32_t i = 0; i < n_bitmap_entries - 1U; i++) {
		EXPECT_EQ(0xFFFFFFFFU, allocated[i]);
	}
	EXPECT_EQ(0x7FFFU, allocated[n_bitmap_entries - 1U]);

	/* We've exhausted the space, further calls should just return
	   n_available */
	EXPECT_EQ(n_available, ALLOC);
	EXPECT_EQ(n_available, ALLOC);

	/* Freeing an entry should allow the allocator to be successful again */
	for (uint32_t i = 0U; i < n_available; i++) {
		FREE(i);
		EXPECT_EQ(n_available - 1U, n_allocated);
		uint32_t idx = ALLOC;
		EXPECT_EQ(i, idx);
		EXPECT_EQ(n_available, n_allocated);
	}

	/* Free all the entries */
	for (uint32_t i = 0U; i < n_available; i++) {
		FREE(i);
		EXPECT_EQ(n_available - i - 1U, n_allocated);
		EXPECT_EQ(0U, free_idx);
	}
}

#define N_THREADS 8U
#define N_REPEAT 25U

static void *_test_mem_alloc_free_threads_main(void *data) {
	for (uint32_t i = 0; i < N_REPEAT; i++) {
		/* Allocate some slots and write to them */
		uint16_t allocations[n_available / N_THREADS];
		for (uint32_t j = 0; j < n_available / N_THREADS; j++) {
			/* Allocate an element */
			allocations[j] = ALLOC;
			EXPECT_GT(n_available, allocations[j]);

			/* Mark the slot as acquired, make sure it has not been acquired
			   at the moment. */
			uint8_t *flag = &slot_acquired[allocations[j]];
			EXPECT_EQ(0U, __atomic_load_n(flag, __ATOMIC_SEQ_CST));
			__atomic_store_n(&slot_acquired[allocations[j]], 1U, __ATOMIC_SEQ_CST);

			/* Increment the slot allocation counter */
			slots[allocations[j]]++;
		}
		/* Free the allocated slots */
		for (uint32_t j = 0; j < n_available / N_THREADS; j++) {
			/* Make sure the slot is allocated at the moment and mark it as
			   unallocated. */
			uint8_t *flag = &slot_acquired[allocations[j]];
			EXPECT_EQ(1U, __atomic_load_n(flag, __ATOMIC_SEQ_CST));
			__atomic_store_n(flag, 0U, __ATOMIC_SEQ_CST);

			/* Deallocate the block */
			FREE(allocations[j]);
		}
	}
	return NULL;
}

static void test_mem_alloc_free_threads(void) {
	pthread_t threads[N_THREADS];

	_test_reset();

	/* Create N_THREADS threads */
	for (uint32_t i = 0U; i < N_THREADS; i++) {
		pthread_create(&threads[i], NULL, _test_mem_alloc_free_threads_main,
		               NULL);
	}

	/* Wait for the threads to finish */
	for (uint32_t i = 0U; i < N_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}

	/* Make sure the total number of allocations performed is correct */
	uint64_t sum = 0;
	for (uint32_t i = 0U; i < n_available; i++) {
		sum += slots[i];
	}
	uint32_t n_allocs = (n_available / N_THREADS) * N_THREADS * N_REPEAT;
	EXPECT_EQ(n_allocs, sum);
}

/******************************************************************************
 * MAIN                                                                       *
 ******************************************************************************/

int main() {
	RUN(test_mem_alloc_free_simple);
	RUN(test_mem_alloc_free_threads);
	DONE;
}

