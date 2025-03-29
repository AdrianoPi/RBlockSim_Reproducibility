# SPDX-FileCopyrightText: 2008-2023 HPDCS Group <rootsim@googlegroups.com>
# SPDX-License-Identifier: GPL-3.0-only

# Retrieve and make available the testing framework
include(FetchContent)
set(IMPORT_AS_LIB ON)
FetchContent_Declare(rstest
        GIT_REPOSITORY https://github.com/ROOT-Sim/testing-framework.git
        GIT_TAG        origin/master
        )
FetchContent_MakeAvailable(rstest)

test_program(init init.c)
test_program_link_libraries(init rsrng)
test_program(numerical numerical.c)
test_program_link_libraries(numerical rsrng)
test_program(xxtea xxtea.c)
test_program_link_libraries(xxtea rsrng)

# TODO: The following is garbage and will be removed soon
target_include_directories(test_numerical PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
target_include_directories(test_xxtea PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
