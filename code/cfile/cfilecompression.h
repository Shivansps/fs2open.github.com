/*
ShivanSpS - Compressed files support for FSO. Many thanks to ngld, taylor and everyone else who helped me in getting this done.

LZ41 FORMAT
--------------------------------------------------------------------------------------
                                  char[4]      (n compressed blocks)   (num_offsets ints)      (int)        (int)              (int)
  COMPRESSED FILE DATA STRUCTURE: HEADER     | BLOCK_0 .. BLOCK_{n-1} | OFFSET_TABLE      | NUM_OFFSETS | ORIGINAL_FILESIZE | BLOCK_SIZE

  * HEADER is a 4-char version tag ("LZ41"). It lets FSO pick the right decoder and lets us add
    future revisions/formats without breaking compatibility. Always 4 chars to keep alignment.
  * Each BLOCK is an *independent* LZ4 block: the compressor resets the HC stream before every
    block, so no block references any other. That is what allows both random access
    and parallel decoding possible.
  * OFFSET_TABLE holds NUM_OFFSETS little-endian ints. offsets[i] is the start of block i within
    the compressed file; offsets[i+1] is its end. The first entry is always 4 (just past the
    header) and the last entry marks the start of the offset table itself. So
    NUM_OFFSETS = (number of blocks) + 1.
  * The 12-byte footer (NUM_OFFSETS, ORIGINAL_FILESIZE, BLOCK_SIZE) is written by the compressor.

  A decoder cache holds the last decoded block so sequential reads only decode each block once.
  All dynamic memory is allocated in comp_create_ci() at cfopen() and released in
  cf_clear_compression_info() at cfclose().
*/

#ifndef _CFILECOMPRESSION_H
#define _CFILECOMPRESSION_H
struct CFILE;

/*LZ41*/
#define LZ41_FILE_HEADER 0x31345A4C // "14ZL" -> "LZ41", inverted
#define LZ41_DECOMPRESSION_ERROR -1
#define LZ41_MAX_BLOCKS_OVERFLOW -2
#define LZ41_HEADER_MISMATCH -3
#define LZ41_OFFSETS_MISMATCH -4
/******/

#define COMP_HEADER_MATCH 1
#define COMP_HEADER_MISMATCH 0

/*
	Returns COMP_HEADER_MATCH if header is a valid compressed file header,
	returns COMP_HEADER_MISMATCH if it dosent.
*/
int comp_check_header(int header);

/*
	This is called to generate the correct compression_info data
	after the file has been indentified as a compressed file.
	This must be done before calling any other function.

	If the file footer or offset table fail validation the file is rejected: compression_info
	is left cleared (header = 0) and cfile considers it as a normal file, so a corrupt
	compressed file never crashes the decoder.
*/
void comp_create_ci(CFILE* cf, int header);

/*
	Read X bytes from the uncompressed file starting from the current position.
	Returns the amount of bytes read, and a negative LZ41_* code (cast to size_t) on error.
*/
size_t comp_fread(CFILE* cf, char* buffer, size_t length);

/*
	Returns the current uncompressed file position.
*/
size_t comp_ftell(CFILE* cf);

/*
	Returns 1 if the uncompressed file has been completely read, otherwise it returns a 0.
*/
int comp_feof(CFILE* cf);

/*
	Used to move the uncompressed file current position.
*/
int comp_fseek(CFILE* cf, int offset, int where);

#endif
