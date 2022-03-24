cmake_minimum_required(VERSION 3.22.3)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(lox_part_2 VERSION 0.1.0)

include(CTest)
enable_testing()


find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS vmdata.proto)
# add_executable(lox_part_2 main.cpp chunk.cpp memory.cpp debug.cpp value.cpp vm.cpp compiler.cpp scanner.cpp object.cpp table.cpp)
add_executable(lox_part_2 main.cpp chunk.cpp memory.cpp debug.cpp value.cpp vm.cpp compiler.cpp scanner.cpp object.cpp table.cpp serialize.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(lox_part_2 ${Protobuf_LIBRARIES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})


set_target_properties(lox_part_2 PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "test_scripts/scope_test.txt")
include(CPack)
