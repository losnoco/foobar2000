/***************************************************************************
                          sidplay2.h  -  Public sidplay header
                             -------------------
    begin                : Fri Jun 9 2000
    copyright            : (C) 2000 by Simon White
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

#ifndef _sidplay2_h_
#define _sidplay2_h_

#include "sidtypes.h"
#include "iinterface.h"
#include "SidTune.h"
#include "sidbuilder.h"


class sidplay2: public IInterface 
{
public:
    static const InterfaceID &iid () {
        return SID2IID<0x25ef79eb, 0x8de6, 0x4076, 0x9c, 0x6b, 0xa9, 0xf9, 0x57, 0x0f, 0x3a, 0x4b>();
    }

    static SID_EXTERN IInterface *create ();

    virtual const sid2_config_t &config (void) const = 0;
    virtual const sid2_info_t   &info   (void) const = 0;

    virtual int            config       (const sid2_config_t &cfg) = 0;
    virtual const char    *error        (void) const = 0;
    virtual int            fastForward  (uint percent) = 0;
    virtual int            load         (SidTune *tune) = 0;
    virtual void           pause        (void) = 0;
    virtual uint_least32_t play         (void *buffer, uint_least32_t length) = 0;
    virtual sid2_player_t  state        (void) const = 0;
    virtual void           stop         (void) = 0;
    virtual void           debug        (bool enable, FILE *out) = 0;

    // Timer functions with respect to resolution returned by timebase
    virtual uint_least32_t timebase (void) const = 0;
    virtual uint_least32_t time     (void) const = 0;
    virtual uint_least32_t mileage  (void) const = 0;
};

// Future interface name
typedef sidplay2 ISidplay2;

#endif // _sidplay2_h_
