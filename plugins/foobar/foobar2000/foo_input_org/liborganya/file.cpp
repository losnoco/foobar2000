/*
 *  file.c
 *  liborganya
 *
 *  Created by Vincent Spader on 6/20/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#include <stdlib.h>
#include <string.h>
#include "file.h"
#include "swap.h"

// File reading helpers 
uint8_t _org_read_8(service_ptr_t<file> & fin, abort_callback & p_abort) {
	uint8_t i = 0;
	fin->read_object_t( i, p_abort );
	return i;
}

uint16_t _org_read_16(service_ptr_t<file> & fin, abort_callback & p_abort) {
	uint16_t i = 0;
	fin->read_object_t( i, p_abort );
	return org_ltoh_16(i);
}

uint32_t _org_read_32(service_ptr_t<file> & fin, abort_callback & p_abort) {
	uint32_t i = 0;
	fin->read_object_t( i, p_abort );
	return org_ltoh_32(i);
}

// Read the usual org header
void _org_read_header(org_header_t *header, service_ptr_t<file> & fin, abort_callback & p_abort)
{
	// Read the magic. All orgyana files start with Org-02.
	int8_t buf[6];
	fin->read_object( buf, 6, p_abort );
	if(0 != memcmp(buf, "Org-02", 6)) {
		throw exception_io_data( "Invalid Organya file signature" );
	}
	
	header->tempo = _org_read_16(fin, p_abort);
	header->steps_per_bar = _org_read_8(fin, p_abort);
	header->beats_per_step = _org_read_8(fin, p_abort);
	header->loop_start = _org_read_32(fin, p_abort);
	header->loop_end = _org_read_32(fin, p_abort);
}

// Read properties for the instrument
void _org_read_instrument(org_instrument_t *instrument, service_ptr_t<file> & fin, abort_callback & p_abort)
{
	instrument->pitch = _org_read_16(fin, p_abort);
	instrument->instrument = _org_read_8(fin, p_abort);
	instrument->disable_sustain = _org_read_8(fin, p_abort);
	instrument->note_count = _org_read_16(fin, p_abort);
}

// Read properties for each note
void _org_read_notes(org_note_t notes[], service_ptr_t<file> & fin, uint16_t note_count, abort_callback & p_abort)
{
	for (uint16_t i = 0; i < note_count; i++) {
		notes[i].start = _org_read_32(fin, p_abort);
	}
	for (uint16_t i = 0; i < note_count; i++) {
		notes[i].key = _org_read_8(fin, p_abort);
	}
	for (uint16_t i = 0; i < note_count; i++) {
		notes[i].length = _org_read_8(fin, p_abort);
	}
	for (uint16_t i = 0; i < note_count; i++) {
		notes[i].volume = _org_read_8(fin, p_abort);
	}
	for (uint16_t i = 0; i < note_count; i++) {
		notes[i].pan = _org_read_8(fin, p_abort);
	}
}

// Rather straightforward just follows the file format.
org_file_t *_org_file_create(service_ptr_t<file> & fin, abort_callback & p_abort) {
	org_file_t *org = ( org_file_t * ) calloc(1, sizeof(org_file_t));
	if ( !org ) throw std::bad_alloc();
	try
	{
		_org_read_header(&org->header, fin, p_abort);

		// Read instrument properties
		for (uint8_t i = 0; i < 16; i++) {
			_org_read_instrument(&org->instruments[i], fin, p_abort);

			// Allocate space for notes
			if (org->instruments[i].note_count) {
				org->instruments[i].notes = ( org_note_t * ) malloc(sizeof(org_note_t) * org->instruments[i].note_count);
				if ( !org->instruments[i].notes ) throw std::bad_alloc();
			}
			else {
				org->instruments[i].notes = NULL;
			}
		}

		// Read notes for each instrument
		for (uint8_t i = 0; i < 16; i++) {
			_org_read_notes(org->instruments[i].notes, fin, org->instruments[i].note_count, p_abort);
		}

		return org;
	}
	catch (...)
	{
		_org_file_destroy( org );
		throw;
	}
}

void _org_file_destroy(org_file_t *org) {
	// Free up memory
	for (uint8_t i = 0; i < 16; i++) {
		if (org->instruments[i].notes) free(org->instruments[i].notes);
	}
	
	free(org);
}
