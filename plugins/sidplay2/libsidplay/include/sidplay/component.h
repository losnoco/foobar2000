/***************************************************************************
                          component.h  -  Standard IC interface functions.
                             -------------------
    begin                : Fri Apr 4 2001
    copyright            : (C) 2001 by Simon White
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

#ifndef _component_h_
#define _component_h_

#include <sidplay/sidunknown.h>

class ISidComponent : public ISidUnknown
{
public:
    static const Iid &iid () {
        return SIDIID<0xa9f9bf8b, 0xd0c2, 0x4dfa, 0x8b8a, 0xf0dd, 0xd7c8b05b>();
    }

    virtual const   char *credits (void) = 0;
    virtual const   char *error   (void) = 0;
    virtual uint8_t read  (uint_least8_t addr) = 0;
    virtual void    reset (void) = 0;
    virtual void    write (uint_least8_t addr, uint8_t data) = 0;
};

#endif // _component_h_
