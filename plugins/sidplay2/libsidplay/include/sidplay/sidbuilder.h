/***************************************************************************
                          sidbuilder.h  -  Sid Builder Interface
                             -------------------
    begin                : Sat May 6 2001
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

#ifndef _sidbuilder_h_
#define _sidbuilder_h_

#include <sidplay/sid2types.h>
#include <sidplay/component.h>
#include <sidplay/c64env.h>

class ISidEmulation: public ISidComponent
{
public:
    static const Iid &iid () {
        return SIDIID<0x82c01032, 0x5d8c, 0x447a, 0x89fa, 0x0599, 0x0990b766>();
    }

    virtual ISidUnknown *builder      (void) const = 0;
    virtual void         clock        (sid2_clock_t clk) = 0;
    virtual void         optimisation (uint_least8_t level) = 0;
    virtual void         reset        (uint_least8_t volume) = 0;

    // @FIXME@ Export via another interface
    virtual int_least32_t output  (uint_least8_t bits) = 0;
};

class ISidMixer: public ISidUnknown
{
public:
    static const Iid &iid () {
        return SIDIID<0xc4438750, 0x06ec, 0x11db, 0x9cd8, 0x0800, 0x200c9a66>();
    }

    virtual void mute   (uint_least8_t num, bool enable) = 0;
    virtual void volume (uint_least8_t num, uint_least8_t level) = 0;
    virtual void gain   (int_least8_t precent) = 0;
};

class ISidBuilder: public ISidUnknown
{
public:
    static const Iid &iid () {
        return SIDIID<0x1c9ea475, 0xac10, 0x4345, 0x8b88, 0x3e48, 0x04e0ea38>();
    }

    virtual operator     bool    () const = 0;
    virtual const char  *credits (void) = 0;
    virtual const char  *error   (void) const = 0;
    virtual ISidUnknown *lock    (c64env *env, sid2_model_t model) = 0;
    virtual void         unlock  (ISidUnknown &device) = 0;
};

#endif // _sidbuilder_h_
