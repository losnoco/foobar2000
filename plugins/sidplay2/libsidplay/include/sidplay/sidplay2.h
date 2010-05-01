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

#include <sidplay/sidtypes.h>
#include <sidplay/sidunknown.h>
#include <sidplay/SidTune.h>
#include <sidplay/sidbuilder.h>


class ISidplay2: public ISidUnknown
{
public:
    static const Iid &iid () {
        SIDIID(0x25ef79eb, 0x8de6, 0x4076, 0x9c6b, 0xa9f9, 0x570f3a4b);
    }

    static SID_EXTERN ISidUnknown *create ();

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
};

class ISidTimer: public ISidUnknown
{
public:
    static const Iid &iid () {
        SIDIID(0xba2f0dd8, 0xdafb, 0x4aea, 0xb09a, 0x8aa9, 0xd335b36b);
    }

    // Timer functions with respect to resolution returned by timebase
    virtual uint_least32_t mileage  (void) const = 0;
//    virtual void           schedule (Event &event, event_clock_t ticks) = 0;
    virtual uint_least32_t timebase (void) const = 0;
    virtual uint_least32_t time     (void) const = 0;
};

// Old name
typedef ISidplay2 sidplay2;

#endif // _sidplay2_h_
