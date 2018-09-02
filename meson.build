#  libfoxenmem -- Utilities for heap-free memory management
#  Copyright (C) 2018  Andreas Stöckel
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Affero General Public License as
#  published by the Free Software Foundation, either version 3 of the
#  License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Affero General Public License for more details.
#
#  You should have received a copy of the GNU Affero General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.

project('libfoxenmem', 'c', default_options : ['c_std=c99'])

# Include directory
inc_foxen = include_directories('.')

# Define the contents of the actual library
lib_foxenmem = library(
    'foxenmem',
    'foxen/mem.c',
    include_directories: inc_foxen,
    install: true)

# Compile and register the unit tests
dep_foxenunit = dependency(
    'libfoxenunit',
    fallback:['libfoxenunit', 'dep_foxenunit'])
exe_test_mem = executable(
    'test_mem',
    'test/test_mem.c',
    include_directories: inc_foxen,
    link_with: lib_foxenmem,
    dependencies: dep_foxenunit,
    install: false)
test('test_mem', exe_test_mem)

dep_threads = dependency('threads')
exe_test_mem_alloc = executable(
    'test_mem_alloc',
    'test/test_mem_alloc.c',
    include_directories: inc_foxen,
    link_with: lib_foxenmem,
    dependencies: [dep_foxenunit, dep_threads],
    install: false)
test('test_mem_alloc', exe_test_mem_alloc)

# Install the header file
install_headers(
    ['foxen/mem.h'],
    subdir: 'foxen')

# Generate a Pkg config file
pkg = import('pkgconfig')
pkg.generate(
    libraries: [lib_foxenmem],
    subdirs: [],
    name: 'libfoxenmem',
    version: '1.0',
    filebase: 'libfoxenmem',
    description: 'Utilities for heap-free memory management')

# Export the dependency
dep_foxenmem = declare_dependency(include_directories: inc_foxen)
