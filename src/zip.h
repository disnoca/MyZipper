#ifndef _ZIP_H
#define _ZIP_H

#include <stdint.h>
#include <windows.h>

#define LOCAL_FILE_HEADER_SIGNATURE 			   	0x04034B50
#define CENTRAL_DIRECTORY_HEADER_SIGNATURE    		0x02014B50
#define END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE   0x06054B50

#define ZIP_VERSION  							  	45
#define WINDOWS_NTFS 							  	0x0A
#define UTF8_ENCODING 							  	(1 << 11)

/* Zip Structs */

typedef struct {
	uint32_t signature;
	uint16_t version;
	uint16_t flags;
	uint16_t compression;
	uint16_t mod_time;
	uint16_t mod_date;
	uint32_t crc32;
	uint32_t compressed_size;
	uint32_t uncompressed_size;
	uint16_t file_name_length;
	uint16_t extra_field_length;
	//char file_name[];
	//unsigned char extra_field[];
} __attribute__((packed)) local_file_header;

typedef struct {
	uint32_t signature;
	uint16_t version_made_by;
	uint16_t version_needed_to_extract;
	uint16_t flags;
	uint16_t compression;
	uint16_t mod_time;
	uint16_t mod_date;
	uint32_t crc32;
	uint32_t compressed_size;
	uint32_t uncompressed_size;
	uint16_t file_name_length;
	uint16_t extra_field_length;
	uint16_t file_comment_length;
	uint16_t disk_number_start;
	uint16_t internal_file_attributes;
	uint32_t external_file_attributes;
	uint32_t local_header_offset;
	//char file_name[];
	//unsigned char extra_field[];
	//char file_comment[];
} __attribute__((packed)) central_directory_header;

typedef struct {
	uint32_t signature;
	uint16_t disk_number;
	uint16_t central_directory_start_disk_number;
	uint16_t num_records_on_disk;
	uint16_t total_num_records;
	uint32_t central_directory_size;
	uint32_t central_directory_start_offset;
	uint16_t comment_length;
	//char comment[];
} __attribute__((packed)) end_of_central_directory_record;


/* ZIP64 Structs */

#define ZIP64_EXTRA_FIELD_HEADER_ID											0x0001

#define ZIP64_END_OF_CENTRAL_DIRECTORY_RECORD_SIGNATURE 					0x06064B50
#define ZIP64_END_OF_CENTRAL_DIRECTORY_LOCATOR_SIGNATURE 					0x07064B50

#define ZIP64_EXTRA_FIELD_FIXED_FIELDS_SIZE 								(sizeof(uint16_t) * 2)
#define ZIP64_END_OF_CENTRAL_DIRECTORY_RECORD_REMAINING_FIXED_FIELDS_SIZE 	(sizeof(uint16_t) * 2 + sizeof(uint32_t) * 2 + sizeof(uint64_t) * 4)

typedef struct {
	uint16_t header_id;
	uint16_t data_size;
	uint64_t extra_fields[3]; 	// uncompressed size, compressed size, local header offset
	//uint32_t disk_number_start;
} __attribute__((packed)) zip64_extra_field;

typedef struct {
	uint32_t signature;
	uint64_t size_of_remaining_zip64_end_of_central_directory_record;
	uint16_t version_made_by;
	uint16_t version_needed_to_extract;
	uint32_t disk_number;
	uint32_t central_directory_start_disk_number;
	uint64_t num_records_on_disk;
	uint64_t total_num_records;
	uint64_t central_directory_size;
	uint64_t central_directory_start_offset;
} __attribute__((packed)) zip64_end_of_central_directory_record;

typedef struct {
	uint32_t signature;
	uint32_t zip64_end_of_central_directory_record_disk_number;
	uint64_t zip64_end_of_central_directory_record_offset;
	uint32_t total_num_disks;
} __attribute__((packed)) zip64_end_of_central_directory_locator;


// Functions

/**
 * Returns the end of central directory record of the specified zip file. If not found, returns an empty struct.
 * 
 * @param zip_name the name of the zip file
 * @param out_eocdr a pointer to a variable to receive the end of central directory record
*/
void find_end_of_central_directory_record(LPWSTR zip_name, end_of_central_directory_record* out_eocdr);

#endif
