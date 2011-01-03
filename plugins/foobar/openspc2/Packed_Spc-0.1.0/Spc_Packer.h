
// Super Nintendo SPC music file packer

// Packed_Spc 0.1.0. Copyright (C) 2004 Shay Green. GNU LGPL license.

#ifndef SPC_PACKER_H
#define SPC_PACKER_H

#include <string.h>

class Spc_Packer {
public:
	Spc_Packer();
	~Spc_Packer();
	
	// If set to false, echo buffer is cleared before compression, resulting in smaller files.
	void preserve_echo_buffer( bool b ) { preserve_echo_buffer_ = b; }
	
	// Initialize and use given filename for shared data and given directory for
	// output. Return NULL on success, otherwise error string.
	const char* init( const char* shared_filename, const char* output_dir );
	
	// Add SPC file data of specified size and filename to set. Return NULL on success,
	// otherwise error string.
	const char* add_spc( const void* data, long size, const char* out_filename );
	
	// Return name of next file to be packed, or NULL if none are left.
	const char* next_file() const;
	
	// Pack current file. Return NULL on success, otherwise error string.
	const char* pack_file();
	
	// Write shared file. Return NULL on success, otherwise error string.
	const char* write_shared();
	
	
	// end of public interface
	
	typedef unsigned char byte;
	
	enum { block_size = 256 };
	enum { ram_size = 0x10000 };
	enum { max_checksum = 255 * block_size };
	typedef unsigned short checksum_t;
private:
	
	struct file_t {
		checksum_t* checksums;
		long size;
		long file_size;
		file_t* next;
		char out_path [256];
		checksum_t checksums_ [ram_size / block_size]; // not used for shared file
		byte file_data [0x100]; // not used for shared file
		byte data [ram_size + 0x1000];
	};
	
	file_t* files;
	file_t* shared;
	
	char output_dir [256];
	bool preserve_echo_buffer_;
	
	class Checksum_Map {
		char bits [max_checksum / 8 + 1];
		byte data [max_checksum + 1];
		enum { no_checksum = 0xFF };
	public:
		Checksum_Map() {
			memset( bits, 0, sizeof bits );
		}
		int check( int n, int d ) const {
			return bits [n >> 3] & (1 << (n & 7)) && (data [n] == d || data [n] == no_checksum);
		}
		void set( int n, int d ) {
			int bit = 1 << (n & 7);
			if ( !(bits [n >> 3] & bit) ) {
				data [n] = d;
				bits [n >> 3] |= bit;
			}
			else if ( data [n] != d ) {
				data [n] = no_checksum;
			}
		}
	};
	
	Checksum_Map shared_map;
	Checksum_Map checksum_map;
	
	byte temp_buf [ram_size + 0x100];
	
	struct result_t {
		file_t* file;
		int skip;
		long offset;
		int size;
	};
	
	long add_shared( const byte* data, unsigned size );
	bool match_data( unsigned sum, byte* begin, byte* pos, byte* end, file_t* file, result_t* );
	byte* match_blocks( byte* out, byte* begin, byte* end, int phase );
};

#endif

