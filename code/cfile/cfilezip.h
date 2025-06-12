/*
	ShivanSpS - ZIP Parser for FSO
*/

#ifndef _CFILEZIP_H
#define _CFILEZIP_H

#include <cstdint>
#include <globalincs/vmallocator.h>

#define ZIP_HEADER_MAGIC 0x04034b50 //Every zip file starts with the LFH signature
#define ZIP_MIN_SIZE	 22

struct zip_pack_file {
	SCP_string file_name_raw;    // File name as stored in the zip file with FSO adjustments, includes the full path. Usefull for folders
    SCP_string file_name;		 // Clean file or folder name, with extension.
	SCP_string file_ext;		 // File extension, only last extension, so something like mytexture.dds.xz will have "xz", do not do that
    size_t local_header_offset;  // Offset to the zip LFH Header structure of this file
    size_t compressed_size;		 // Actual data length in the zip file
    size_t uncompressed_size;	 // Original file size before compression
    uint16_t compression_method; // STORE = 0, XZ = 95
    uint8_t type;                // Directory = 0, File = 1
};

enum zip_ret {
    ZIP_OK,
    ZIP_NOT_FOUND,
    ZIP_ERROR
};

/*
* Gets a full file list in "zip_pack_file" structure format of all files and folders inside the zip file
* ZIP64 is supported provided the code is using 64 bit capable seeks.
* Returns ZIP_OK or ZIP_ERROR.
*/
zip_ret zip_get_files(FILE* fp, const SCP_string& zip_name, SCP_vector<zip_pack_file>* files);

/*
* Files inside zip packs contain the Local File Header offset in their lib_offset instead
* of the actual data offset, calling this function will read the Local File Header and
* store the real data offset in real_offset
* Returns ZIP_OK or ZIP_ERROR
*/
zip_ret zip_get_real_data_offset(FILE* fp, size_t* lib_offset);
#endif
