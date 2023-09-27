cmake_minimum_required(VERSION 3.0.0)
project(MyZipper VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

add_library(global_lib SHARED wrapper_functions.c wrapper_functions.h utils.h)

add_library(zip_lib SHARED zip.c zip.h)
target_link_libraries(zip_lib PRIVATE global_lib)

add_library(compression_lib SHARED compression/compression.h 
	compression/concurrency.c compression/concurrency.h 
	compression/crc32.c compression/crc32.h 
	compression/no_compression/no_compression.c)
target_link_libraries(compression_lib PRIVATE global_lib zip_lib)


add_library(zipper_lib STATIC
	zipper/queue.c zipper/queue.h
	zipper/zipper_file.c zipper/zipper_file.h)
target_link_libraries(zipper_lib PRIVATE global_lib zip_lib compression_lib)


add_executable(zipper zipper/zipper.c)
target_link_libraries(zipper PRIVATE global_lib zip_lib compression_lib zipper_lib)

add_executable(unzipper unzipper/unzipper.c)
target_link_libraries(unzipper PRIVATE global_lib zip_lib compression_lib)

add_executable(zip_info zip_info/zip_info.c)
target_link_libraries(zip_info PRIVATE global_lib zip_lib)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
