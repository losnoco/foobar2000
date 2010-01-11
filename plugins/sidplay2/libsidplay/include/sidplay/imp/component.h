/***************************************************************************
                          imp_component.h  -  Component Implementation
                             -------------------
    begin                : Sat Jun 6 2006
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

#ifndef _imp_component_h_
#define _imp_component_h_

// This file is only to be used be things creating implementations.
// DO NOT include this file in external code usings these implementations,
// use interface files instead:

#include <string.h>
#include <sidplay/component.h>

//-------------------------------------------------------------------------
#include <sidplay/imp/iinterface.h>

template <class TImplementation>
class Component: public ICoInterface<TImplementation>
{
public:
    Component (const char *name) : ICoInterface<TImplementation>(name) {;}
    virtual ~Component () {;}

    // IInterface
    void _ifrelease () { if (!this->_ifrefcount()) delete this; }
};

#endif // _imp_component_h_
