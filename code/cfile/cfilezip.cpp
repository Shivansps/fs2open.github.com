/*
	ShivanSpS - ZIP Parser for FSO
*/
#define _CFILE_INTERNAL
#include "cfilezip.h"

#include <cstdlib>
#include <cstring>

#define ZIP_LFH_SIGNATURE            0x04034b50
#define ZIP_CDFH_SIGNATURE           0x02014b50
#define ZIP_EOCD_SIGNATURE           0x06054b50
#define ZIP64_EOCD_LOCATOR_SIGNATURE 0x07064b50
#define ZIP64_EOCD_SIGNATURE         0x06064b50
#define ZIP64_EXTRA_FIELD_SIGNATURE  0x0001

// ZIP structures
#pragma pack(push, 1)
struct Local_File_Header {
    uint32_t signature;                 //0x04034b50
    uint16_t version;
    uint16_t flags;
    uint16_t compression_method;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_length;
    uint16_t extra_field_length;
};

struct Central_Directory_File_Header {
    uint32_t signature;                 // 0x02014b50
    uint16_t version_archiver;          // Version of the archiver
    uint16_t minimum_version;           // Minimum version to extract
    uint16_t flags;                     // General purpose bit flag
    uint16_t compression_method;        // Compression method
    uint16_t modified_time;             // File last modification time
    uint16_t modified_date;             // File last modification date
    uint32_t crc32;                     // CRC-32 checksum
    uint32_t compressed_size;           // Compressed size
    uint32_t uncompressed_size;         // Uncompressed size
    uint16_t filename_length;           // Filename length
    uint16_t extra_field_length;        // Extra field length
    uint16_t file_comment_length;       // File comment length
    uint16_t disk_number_start;         // Disk number start
    uint16_t internal_file_attributes;  // Internal file attributes
    uint32_t external_file_attributes;  // External file attributes
    uint32_t local_header_offset;       // Relative offset of local file header from start of file
};

struct EOCD {
    uint32_t signature;                 // 0x06054b50
    uint16_t disk_number;               // Number of this disk
    uint16_t cd_disk_number;            // Number of the disk with central directory
    uint16_t num_entries_on_this_disk;  // Number of central directory entries on this disk
    uint16_t num_entries_total;         // Total number of central directory entries
    uint32_t cd_size;                   // Size of central directory
    uint32_t cd_offset;                 // Offset of start of central directory from start of file
    uint16_t comment_length;            // ZIP file comment length
};

// ZIP64 Structures
struct Zip64_EOCD_Locator {
    uint32_t signature;                 // 0x07064b50
    uint32_t disk_number;
    uint64_t offset_to_EOCD;            // Offset of start of central directory from start of file
    uint32_t total_disks;
};

struct Zip64_EOCD {
    uint32_t signature;                 //0x06064b50
    uint64_t size_of_this_record;
    uint16_t version_made_by;
    uint16_t version_needed_to_extract;
    uint32_t disk_number;
    uint32_t disk_with_start_of_cd;
    uint64_t num_entries_on_this_disk;
    uint64_t num_entries_total;
    uint64_t cd_size;
    uint64_t cd_offset;
};
#pragma pack(pop)

static inline uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static inline uint64_t rd64(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 7; i >= 0; --i) v = (v << 8) | p[i];
    return v;
}

size_t zip_ftell(FILE* fp)
{
	#ifdef _WIN32
	return (size_t)_ftelli64(fp);
	#else
	return (size_t)ftello(fp);
	#endif
}

int zip_seek(FILE* fp, int64_t offset, int mode)
{
	#ifdef _WIN32
	return _fseeki64(fp, offset, mode);
	#else
	return fseeko(fp, (off_t)offset, mode);
	#endif
}

size_t zip_read(void* buffer, size_t elem_size, size_t elem_num, FILE* fp)
{
    return fread(buffer, elem_size, elem_num, fp);
}

SCP_string get_clean_file_name(const SCP_string& str)
{
	size_t start_pos = str.find_last_of('/');
	if (start_pos == std::string::npos) {
		start_pos = 0;
	} else {
		start_pos++;
	}
	if (start_pos < str.length())
		return str.substr(start_pos, str.length());
	else
		return str;
}

SCP_string get_file_ext(const SCP_string& str)
{
	size_t start_pos = str.find_last_of('.');
	if (start_pos == std::string::npos) {
		start_pos = 0;
	} else {
		start_pos++;
	}
	if (start_pos < str.length())
		return str.substr(start_pos, str.length());
	else
		return str;
}

void replace_all_occurrences(SCP_string& str, char old, char newc)
{
	for (char& c : str) {
		if (c == old) {
			c = newc;
		}
	}
}

zip_ret zip_get_EOCD(EOCD* eocd, size_t* eocd_pos, FILE* fp)
{
	zip_seek(fp, 0, SEEK_END);
	size_t file_size = zip_ftell(fp);
	if (file_size < sizeof(EOCD))                 // too small to hold an EOCD -> not a zip
		return ZIP_NOT_FOUND;

	size_t cache_size = file_size < 65536 ? file_size : 65536;
	auto buffer = (char*)malloc(cache_size);
	if (buffer == nullptr)
		return ZIP_ERROR;

	zip_seek(fp, -(int64_t)cache_size, SEEK_END);
	if (zip_read(buffer, 1, cache_size, fp) != cache_size) {
		free(buffer);
		return ZIP_ERROR;
	}

	static const uint8_t SIG[4] = { 0x50, 0x4b, 0x05, 0x06 };  // 0x06054b50, little-endian
	// Scan backward for the last EOCD signature. `k-- > 0` tests k from (cache_size - sizeof(EOCD))
	// down to 0 inclusive and then stops, with no unsigned wraparound.
	for (size_t k = cache_size - sizeof(EOCD) + 1; k-- > 0; ) {
		if (memcmp(buffer + k, SIG, 4) == 0) {
			memcpy(eocd, buffer + k, sizeof(EOCD)); // ignore the trailing comment
			*eocd_pos = cache_size - k;             // bytes from EOCD start to EOF
			free(buffer);
			return ZIP_OK;
		}
	}
	free(buffer);
	return ZIP_NOT_FOUND;
}

zip_ret zip_get_LFH(FILE* fp, size_t offset, Local_File_Header* lfh) {
    zip_seek(fp, offset, SEEK_SET);
    if (zip_read(lfh, sizeof(Local_File_Header), 1, fp) != 1)   // [FIX 4]
        return ZIP_ERROR;
    if (lfh->signature == ZIP_LFH_SIGNATURE)
        return ZIP_OK;
    return ZIP_NOT_FOUND;
}

zip_ret zip_get_real_data_offset(FILE* fp, size_t* lib_offset)
{
	Local_File_Header lfh;
	// Per spec the LFH lengths may differ from the CDFH ones, so the real data offset must be
	// computed from the LFH itself (this is why it is deferred to open time).
	if (zip_get_LFH(fp, *lib_offset, &lfh) == ZIP_OK)
	{
		*lib_offset += sizeof(Local_File_Header) + lfh.filename_length + lfh.extra_field_length;
		return ZIP_OK;
	}
	return ZIP_ERROR;
}

zip_ret zip_get_CDFH(size_t offset, uint64_t entries, FILE* fp, SCP_vector<zip_pack_file>* files) {
    zip_seek(fp, offset, SEEK_SET);
    for (; entries > 0; entries--)
    {
        Central_Directory_File_Header fh;
        if (zip_read(&fh, sizeof(Central_Directory_File_Header), 1, fp) != 1)   // [FIX 4]
            return ZIP_ERROR;
        if (fh.signature != ZIP_CDFH_SIGNATURE)
            return ZIP_ERROR;

        zip_pack_file zf;
        if (fh.filename_length > 0)
        {
            char* fn = (char*)malloc(fh.filename_length + 1);
            if (fn == nullptr)
                return ZIP_ERROR;
            if (zip_read(fn, fh.filename_length, 1, fp) != 1) {
                free(fn);
                return ZIP_ERROR;
            }
            fn[fh.filename_length] = '\0';
            zf.file_name_raw = SCP_string(fn, fh.filename_length);

            // All folders in zip end with a "/"
            if (fn[fh.filename_length - 1] == '/') {
                zf.type = 0;
                zf.file_name_raw.pop_back(); // and lets get rid of that
                replace_all_occurrences(zf.file_name_raw, '/', DIR_SEPARATOR_CHAR); // FSO folder separators
            } else {
                zf.type = 1;
                zf.file_ext = get_file_ext(zf.file_name_raw); // FSO needs the extension anyway
            }

            zf.file_name = get_clean_file_name(zf.file_name_raw);
            free(fn);
        }

        zf.compressed_size     = fh.compressed_size;
        zf.local_header_offset = fh.local_header_offset;
        zf.uncompressed_size   = fh.uncompressed_size;
        zf.compression_method  = fh.compression_method;

        // Read the whole extra field block (bounded by uint16, <= 65535 bytes) so it can be
        // walked in memory; then skip the comment.
        uint8_t* extra = nullptr;
        if (fh.extra_field_length > 0) {
            extra = (uint8_t*)malloc(fh.extra_field_length);
            if (extra == nullptr)
                return ZIP_ERROR;
            if (zip_read(extra, 1, fh.extra_field_length, fp) != fh.extra_field_length) {
                free(extra);
                return ZIP_ERROR;
            }
        }

        // ZIP64: present only when one of the 32-bit fields uses the 0xFFFFFFFF sentinel. The
        // ZIP64 extra record (0x0001) is NOT guaranteed to be first, so walk all extra fields.
        if (fh.compressed_size == 0xFFFFFFFF || fh.uncompressed_size == 0xFFFFFFFF ||
            fh.local_header_offset == 0xFFFFFFFF)
        {
            bool found = false;
            size_t p = 0;
            while (extra != nullptr && p + 4 <= (size_t)fh.extra_field_length) {
                uint16_t id   = rd16(extra + p);
                uint16_t dsz  = rd16(extra + p + 2);
                if (p + 4 + (size_t)dsz > (size_t)fh.extra_field_length)
                    break;                                  // malformed extra field -> stop, no OOB
                if (id == ZIP64_EXTRA_FIELD_SIGNATURE) {
                    size_t q = p + 4, end = q + dsz;
                    // Fields appear in this fixed order, each only if its 32-bit field was the sentinel.
                    if (fh.uncompressed_size   == 0xFFFFFFFF && q + 8 <= end) { zf.uncompressed_size   = rd64(extra + q); q += 8; }
                    if (fh.compressed_size     == 0xFFFFFFFF && q + 8 <= end) { zf.compressed_size     = rd64(extra + q); q += 8; }
                    if (fh.local_header_offset == 0xFFFFFFFF && q + 8 <= end) { zf.local_header_offset = rd64(extra + q); q += 8; }
                    // (disk_number_start, 4 bytes, may follow if 0xFFFF; unused by FSO.)
                    found = true;
                    break;
                }
                p += 4 + dsz;
            }
            if (!found) {                                   // sentinels but no ZIP64 record -> malformed
                free(extra);
                return ZIP_ERROR;
            }
        }

        free(extra);

        // Skip the file comment to land on the next CDFH entry.
        if (fh.file_comment_length > 0)
            zip_seek(fp, fh.file_comment_length, SEEK_CUR);

        files->emplace_back(zf);
    }
    return ZIP_OK;
}

zip_ret zip64_get_EOCD(Zip64_EOCD* eocd, size_t offset, FILE* fp)
{
    zip_seek(fp, offset, SEEK_SET);
    if (zip_read(eocd, sizeof(Zip64_EOCD), 1, fp) != 1)     // [FIX 4]
        return ZIP_ERROR;
    if (eocd->signature == ZIP64_EOCD_SIGNATURE)
        return ZIP_OK;
    return ZIP_NOT_FOUND;
}

zip_ret zip64_get_eocd_locator(FILE* fp, Zip64_EOCD_Locator* locator, size_t eocd_pos)
{
    zip_seek(fp, -(int64_t)(sizeof(Zip64_EOCD_Locator) + eocd_pos), SEEK_END);
    if (zip_read(locator, sizeof(Zip64_EOCD_Locator), 1, fp) != 1)   // [FIX 4]
        return ZIP_ERROR;
    if (locator->signature == ZIP64_EOCD_LOCATOR_SIGNATURE)
        return ZIP_OK;
    return ZIP_NOT_FOUND;
}

zip_ret zip_get_files(FILE* fp, const SCP_string& zip_name, SCP_vector<zip_pack_file>* files)
{
    EOCD eocd;
    size_t eocd_pos = 0;
	//All zips, be 32bits or 64bits have a EOCD, find it
    if (zip_get_EOCD(&eocd, &eocd_pos, fp) != ZIP_OK) {
		mprintf(("Unable to parse the EOCD record of the zip file ('%s')...\n", zip_name.c_str()));
        return ZIP_ERROR;
    }

    size_t cdfh_offset = eocd.cd_offset;
    uint64_t num_files = eocd.num_entries_total;

	// Use ZIP64 when the central directory size/offset OR the entry count hits its 32/16-bit
	// sentinel. 
    if (eocd.cd_size == 0xFFFFFFFF || eocd.cd_offset == 0xFFFFFFFF || eocd.num_entries_total == 0xFFFF) {
        //ZIP64 use the locator to find the EOCD, and finally the CDFH
        Zip64_EOCD_Locator eocd_locator;
        if (zip64_get_eocd_locator(fp, &eocd_locator, eocd_pos) != ZIP_OK) {
			mprintf(("Unable to parse the EOCD Locator record of the zip64 file ('%s')...\n", zip_name.c_str()));
            return ZIP_ERROR;
        }
        Zip64_EOCD eocd64;
        if (zip64_get_EOCD(&eocd64, eocd_locator.offset_to_EOCD, fp) != ZIP_OK) {
			mprintf(("Unable to parse the EOCD record of the zip64 file ('%s')...\n", zip_name.c_str()));
            return ZIP_ERROR;
        }
        cdfh_offset = eocd64.cd_offset;
        num_files = eocd64.num_entries_total;
    }

	// Parse CDFH to get the files info
    if (zip_get_CDFH(cdfh_offset, num_files, fp, files) != ZIP_OK) {
		mprintf(("Unable to parse the CDFH record of the zip file ('%s')...\n", zip_name.c_str()));
        return ZIP_ERROR;
    }

	// I Can't Believe It's Not a TAR!, it turns out the index (CDFH) dosent contain all data needed to point to the raw file data as needed.
	// Because information in the Local File Header dosent have to match the one in the CDFH per spec and some on them are of variable size,
	// like extra_field_length. So we need to actually seek and pase the local file header of EACH FILE to find the right offset to the data.
	// This will be done deeper in the code when the files are opened to avoid having to parse the entire file here.

    return ZIP_OK;
}