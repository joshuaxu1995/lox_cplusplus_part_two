cmake_minimum_required(VERSION 3.0.0)
project(lox_part_2 VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(lox_part_2 main.cpp chunk.cpp memory.cpp debug.cpp value.cpp vm.cpp compiler.cpp scanner.cpp object.cpp table.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set_target_properties(lox_part_2 PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "test_scripts/scope_test.txt")
include(CPack)
