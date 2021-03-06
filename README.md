# libfoxenmem ― Utilities for heap-free memory management

[![Build Status](https://travis-ci.org/astoeckel/libfoxenmem.svg?branch=master)](https://travis-ci.org/astoeckel/libfoxenmem)

This library provides a couple of utility functions and macros for heap-free
memory allocation and alignment shared with other `libfoxen` libraries.

## Usage

### Example

Consider the following data structure for a complex (in the mathematical sense)
matrix type.
```C
struct complex_matrix {
	uint16_t w, h;
	float *real, *imag;
};

typedef struct complex_matrix complex_matrix_t;
```
This data structure consists of “bookkeeping” data (width/height)
as well as two arrays holding the real and imaginary part. We want to
store the bookkeeping data and the imaginary and real matrices in a
*single contiguous memory region*. Furthermore, the arrays should
start at aligned addresses.

**Two tasks:**
1. Compute the *total required size* of the contiguous memory region.
2. Given a pointer at a memory region compute *aligned pointers* at the
   bookkeeping data as well as the payload.

#### Computing the size

```C
uint32_t complex_matrix_size(uint16_t width, uint16_t height) {
	uint32_t size;
	bool ok = fx_mem_init_size(&size) &&
	          fx_mem_update_size(&size, sizeof(complex_matrix_t)) &&
	          fx_mem_update_size(&size, sizeof(float) * width * height) &&
	          fx_mem_update_size(&size, sizeof(float) * width * height);
	return ok ? size : 0; /* ok is false if there was an overflow */
}
```

#### Computing the individual pointers

```C
complex_matrix_t *complex_matrix_init(void *mem, uint16_t width,
                                      uint16_t height) {
	complex_matrix_t *mat =
	    (complex_matrix_t *)fx_mem_align(&mem, sizeof(complex_matrix_t));
	mat->w = width, mat->h = height;
	mat->real = (float *)fx_mem_align(&mem, width * height);
	mat->imag = (float *)fx_mem_align(&mem, width * height);
	return mat;
}
```

### Macros

`mem.h` defines the following macros

---

`FX_ALIGN`<br/>
Alignment boundary in bytes. Should be 16 to support SSE.

---

`FX_ASSUME_ALIGNED(P)`<br/>
Tells the compiler that it should assume that `P` is aligned at a 16-byte
boundary. May allow the compiler to emit more efficient code.

---

`FX_ALIGN_ADDR(P)`<br/>
Aligns the pointer `P` at a 16 byte boundary.

---

`FX_MEM_ZERO_ALIGNED(P)`<br/>
Macro that fills the structure pointed at by `P` with zeros. See
`fx_mem_zero_aligned()` regarding potential dangers.

### Functions

```C
static inline bool fx_mem_init_size(uint32_t *size);
```
Call this first in a chain of fx_mem_update_size() calls. It will make sure
that there is enough space to align the datastructure whenever the user
provides a non-aligned target memory pointer.

**Parameters:**<br/>
*size* is a pointer at a variable that holds the size of the object that we're
describing. This function initializes this value to `FX_ALIGN - 1`.

**Return value:**<br/>
Always returns true to facilitate chaining with other `fx_mem_*_size` functions.

---

```C
static inline bool fx_mem_update_size(uint32_t *size, uint32_t n_bytes);
```
Function used internally to compute the total size of a data structure
consisting of multiple substructure. Calling this function updates the size
of the outer data structure by adding a substructure of size `n_bytes`. Assumes
that the beginning of the substructure should be aligned.

**Parameters:**<br/>
*size* is a pointer at the variable holding the size of the data structure.<br/>
*n_bytes* size of the sub-structure that should be added.

**Return value:**<br/>
Zero if there was an overflow, one otherwise.

---

```C
static inline void fx_mem_zero_aligned(void *mem, uint32_t size);
```
Fills the given memory region with zeros. In contrast to `memset(mem, 0, size)`
this assumes that the pointer is aligned at the `FX_ALIGN` boundary and that
we can write multiples of `FX_ALIGN` bytes at once. This is **potentially**
**dangerous**, so do not use this function if you don't know exactly what you're
doing.

**Parameters:**<br/>
*mem* is a pointer at the memory region that should be zeroed out. This
pointer is assumed to be aligned.<br/>
*size* is the size of the memory region that should be zeroed in bytes.
This value is effectively rounded up to a multiple of `FX_ALIGN`.<br/>

---

```C
static inline void* fx_mem_align(void **mem, uint32_t size);
```
Computes the aligned pointer pointing at the substructure of the given size.

**Parameters:**<br/>
*mem* pointer at the variable holding the pointer at the pointer that should be computed. The pointer is advanced by the given size after the return value is computed.<br/>
*size* size of the substructure for which the pointer should be returned.

**Return value:**<br/>
An aligned pointer pointing at the beginning of the substructure.


## FAQ about the *Foxen* series of C libraries

**Q: What's with the name?**

**A:** [*Foxen*](http://kingkiller.wikia.com/wiki/Foxen) is a mysterious glowing object guiding Auri through the catacumbal “Underthing”. The *Foxen* software libraries are similar in key aspects: mysterious and catacumbal. Probably less useful than an eternal sympathy lamp though.

**Q: What is the purpose and goal of these libraries?**

**A:** The *Foxen* libraries are extremely small C libraries that rely on the [Meson](https://mesonbuild.com/) build system for dependency management. One common element is that the libraries do not use [heap memory allocations](https://github.com/astoeckel/libfoxenmem). They can thus be easily compiled to tiny, standalone [WASM](https://webassembly.org/) code.

**Q: Why?**

**A:** Excellent question! The author mainly created these libraries because he grew tired of copying his own source code files between projects all the time.

**Q: Would you recommend to use these libraries in my project?**

**A:** That depends. Some of the code is fairly specialized according to my own needs and might not be intended to be general. If what you are going to use these libraries for aligns with their original purpose, then sure, go ahead. Otherwise, I'd probably advise against using these libraries, and as explained below, I'm not super open to expanding their scope.

**Q: Can you licence these libraries under a something more permissive than AGPLv3?**

**A:** Maybe, if you ask nicely. I'm not a fan of giving my work away “for free” (i.e., allowing inclusion of my code in commercial or otherwise proprietary software) without getting something back (in particular public access to the source code of the things other people built with it). That being said, some of the `foxen` libraries may be too trivial to warrant the use of a strong copyleft licence. Correspondingly, I might reconsider this decision for individual libraries. See “[Why you shouldn't use the Lesser GPL for your next library](https://www.gnu.org/licenses/why-not-lgpl.en.html)” for more info.

**Q: Can I contribute?**

**A:** Sure! Feel free to open an issue or a PR. However, be warned that since I've mainly developed these libraries for use in my own stuff, I might be a little picky about what I'm going to include and what not.

## Licence

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.
