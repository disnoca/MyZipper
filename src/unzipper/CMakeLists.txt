add_executable(unzipper unzipper.c)
target_link_libraries(unzipper PRIVATE global_lib zip_lib my_compression_lib)
