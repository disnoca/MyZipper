add_library(my_compression_lib SHARED compression.h 
	concurrency.c concurrency.h 
	crc32.c crc32.h 
	no_compression/no_compression.c)

target_link_libraries(my_compression_lib PRIVATE global_lib zip_lib)
