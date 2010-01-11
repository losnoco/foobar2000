/***************************************************************************
                          imp_sidbuilder.h  -  Sid Builder Implementation
                             -------------------
    begin                : Sat June 17 2006
    copyright            : (C) 2006 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _imp_sidbuilder_h_
#define _imp_sidbuilder_h_

// This file is only to be used be things creating implementations.
// DO NOT include this file in external code usings these implementations,
// use interface files instead:

#include <sidplay/sidbuilder.h>

//-------------------------------------------------------------------------
#include <sidplay/imp/component.h>

// Inherit this class to create a new SID emulations for libsidplay2.
template <class TImplementation, class TBuilder = ISidBuilder>
class SidEmulation: public Component<TImplementation>
{
private:
    TBuilder * const m_builder;

private:
    void reset (void) { reset (0); }

public:
    SidEmulation (const char *name, TBuilder *builder)
    :Component<TImplementation>(name), m_builder (builder) {;}

    // ISidEmulation
    ISidBuilder         *builder      (void) const { return m_builder; }
    virtual void         clock        (sid2_clock_t clk) {;}
    virtual void         gain         (int_least8_t precent) {;}
    virtual void         optimisation (uint_least8_t level) {;}
    virtual void         reset        (uint_least8_t volume) = 0;
};

template <class TImplementation>
class SidBuilder: public ICoInterface<TImplementation>
{
protected:
    bool m_status;

public:
    // Determine current state of object (true = okay, false = error).
    SidBuilder(const char * name)
    : ICoInterface<TImplementation>(name), m_status (true) {;}

    // IInterface
    void _ifdestroy () { delete this; }

    // ISidBuilder (implementations only)
    operator bool   () const { return m_status; }
};

#endif // _imp_sidbuilder_h_
