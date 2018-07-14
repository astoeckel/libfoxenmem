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

#include <foxen/mem.h>
#include <foxen/unittest.h>

/******************************************************************************
 * CODE EXAMPLE FROM README.MD                                                *
 ******************************************************************************/

struct complex_matrix {
	uint16_t w, h;
	float *real, *imag;
};

typedef struct complex_matrix complex_matrix_t;

uint32_t complex_matrix_size(uint16_t width, uint16_t height) {
	uint32_t size;
	bool ok = fx_mem_init_size(&size) &&
	          fx_mem_update_size(&size, sizeof(complex_matrix_t)) &&
	          fx_mem_update_size(&size, sizeof(float) * width * height) &&
	          fx_mem_update_size(&size, sizeof(float) * width * height);
	return ok ? size : 0; /* ok is false if there was an overflow */
}

complex_matrix_t *complex_matrix_init(void *mem, uint16_t width,
                                      uint16_t height) {
	complex_matrix_t *mat =
	    (complex_matrix_t *)fx_mem_align(&mem, sizeof(complex_matrix_t));
	mat->w = width, mat->h = height;
	mat->real = (float *)fx_mem_align(&mem, width * height);
	mat->imag = (float *)fx_mem_align(&mem, width * height);
	return mat;
}

/******************************************************************************
 * UNIT TESTS                                                                 *
 ******************************************************************************/

void test_align_addr() {
	EXPECT_EQ(0xABC0U, (uintptr_t)FX_ALIGN_ADDR(0xABC0U));
	for (int i = 1; i <= FX_ALIGN; i++) {
		EXPECT_EQ(0xABD0U, (uintptr_t)FX_ALIGN_ADDR(0xABC0U + i));
	}
}

void test_mem_update_size_simple_1() {
	uint32_t size;
	EXPECT_TRUE(fx_mem_init_size(&size));
	EXPECT_EQ(FX_ALIGN, size);
	EXPECT_TRUE(fx_mem_update_size(&size, 12));
	EXPECT_EQ(2 * FX_ALIGN, size);
	EXPECT_TRUE(fx_mem_update_size(&size, 12));
	EXPECT_EQ(3 * FX_ALIGN, size);
}

void test_mem_update_size_simple_2() {
	uint32_t size;
	EXPECT_TRUE(fx_mem_init_size(&size));
	EXPECT_EQ(FX_ALIGN, size);
	EXPECT_TRUE(fx_mem_update_size(&size, 1));
	EXPECT_EQ(2 * FX_ALIGN, size);
}

void test_mem_update_size_simple_3() {
	uint32_t size;
	EXPECT_TRUE(fx_mem_init_size(&size));
	EXPECT_EQ(FX_ALIGN, size);
	EXPECT_TRUE(fx_mem_update_size(&size, 0));
	EXPECT_EQ(FX_ALIGN, size);
}

void test_mem_update_size_overflow() {
	uint32_t size = 0;
	EXPECT_TRUE(fx_mem_update_size(&size, 0xFFFFFFFEU));

	size = 1;
	EXPECT_FALSE(fx_mem_update_size(&size, 0xFFFFFFFEU));
}

void test_example_code() {
	uint8_t mem[1024];

	ASSERT_GT(1024, complex_matrix_size(8, 8));
	ASSERT_LT(0, complex_matrix_size(8, 8));

	complex_matrix_t *mat = complex_matrix_init(mem, 8, 8);
	EXPECT_EQ(0U, (uintptr_t)(mat) & 0xF);
	EXPECT_TRUE(mat != NULL);
	EXPECT_TRUE(mat->real != NULL);
	EXPECT_TRUE(mat->imag != NULL);
	EXPECT_TRUE(mat->real != mat->imag);
	EXPECT_GE(256U, (uintptr_t)(mat->imag) - (uintptr_t)(mat->imag));
	EXPECT_EQ(0U, (uintptr_t)(mat->real) & 0xF);
	EXPECT_EQ(0U, (uintptr_t)(mat->imag) & 0xF);
}

/******************************************************************************
 * MAIN                                                                       *
 ******************************************************************************/

int main() {
	RUN(test_align_addr);
	RUN(test_mem_update_size_simple_1);
	RUN(test_mem_update_size_simple_2);
	RUN(test_mem_update_size_simple_3);
	RUN(test_example_code);
	DONE;
}

