add_executable(zipper zipper.c zipper_file.c queue.c)
target_link_libraries(zipper PRIVATE global_lib zip_lib my_compression_lib)
