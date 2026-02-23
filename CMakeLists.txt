cmake_minimum_required(VERSION 3.10)

project(sdfs C)

add_executable(dummy src/main.c)

target_include_directories(dummy PRIVATE
	"${CMAKE_SOURCE_DIR}/inc"
)

target_compile_definitions(dummy PRIVATE)
