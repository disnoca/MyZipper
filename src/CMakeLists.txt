add_library(global_lib STATIC wrapper_functions.c wrapper_functions.h utils.h)

add_library(zip_lib STATIC zip.c zip.h)
target_link_libraries(zip_lib PRIVATE global_lib)

add_subdirectory(compression)
add_subdirectory(zipper)
add_subdirectory(unzipper)
add_subdirectory(zip_info)
