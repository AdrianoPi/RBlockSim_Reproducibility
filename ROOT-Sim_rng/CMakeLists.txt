# SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

cmake_minimum_required(VERSION 3.12)
project("ROOT-RNG" LANGUAGES C DESCRIPTION "The ROOT-Sim Random Number Generators library")

set(PROJECT_VERSION 1.0.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)

add_compile_options(-Wall -Wextra -pedantic)

add_subdirectory(src)

if(NOT IMPORT_AS_LIB)
    # Run the tests
    enable_testing()
    add_subdirectory(test)
endif()
