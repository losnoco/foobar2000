
// Super Nintendo (SNES) SPC-700 CPU emulator

// Spc_Emu 0.1.0. Copyright (C) 2004 Shay Green. GNU GPL license.

#ifndef SPC_CPU_H
#define SPC_CPU_H

typedef unsigned spc_addr_t;
typedef long spc_time_t;

class Spc_Emu;

class Spc_Cpu {
	typedef unsigned char uint8_t;
	typedef signed char int8_t;
	uint8_t* const ram;
	spc_time_t remain_;
	Spc_Emu& emu;
public:
	// Use the 64KB RAM provided
	Spc_Cpu( uint8_t ram [0x10000], Spc_Emu* );
	
	// SPC-700 registers. *Not* kept updated during a call to run().
	struct registers_t {
		unsigned short pc;
		uint8_t a;
		uint8_t x;
		uint8_t y;
		uint8_t status;
		uint8_t sp;
	} r;
	
	// Run CPU for at least 'count' cycles. Return the number of cycles remaining
	// when emulation stopped (negative if extra cycles were emulated). Emulation
	// stops when there are no more remaining cycles or an unhandled instruction
	// is encountered (STOP, SLEEP, and any others not yet implemented). In the
	// latter case, the return value is greater than zero.
	spc_time_t run( spc_time_t count );
	
	// Number of clock cycles remaining for current run() call.
	spc_time_t remain() const;
	
	// Access memory as the emulated CPU does.
	int  read ( spc_addr_t );
	void write( spc_addr_t, int );
	
private:
	// noncopyable
	Spc_Cpu( const Spc_Cpu& );
	Spc_Cpu& operator = ( const Spc_Cpu& );
	unsigned mem_bit( spc_addr_t );
	
	static const uint8_t CycleTable[256];
};

	inline spc_time_t Spc_Cpu::remain() const {
		return remain_;
	}

#endif

