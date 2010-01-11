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

#include "sid2types.h"
#include "component.h"
#include "c64env.h"

class ISidBuilder;

class ISidEmulation: public IComponent
{
public:
    static const InterfaceID &iid () {
        return SID2IID<0x82c01032, 0x5d8c, 0x447a, 0x89, 0xfa, 0x05, 0x99, 0x09, 0x90, 0xb7, 0x66>();
    }

    virtual ISidBuilder *builder      (void) const = 0;
    virtual void         clock        (sid2_clock_t clk) = 0;
    virtual void         optimisation (uint_least8_t level) = 0;
    virtual void         reset        (uint_least8_t volume) = 0;

    // @FIXME@ Export via another interface
    virtual int_least32_t output  (uint_least8_t bits) = 0;
};

class ISidMixer: public IInterface
{
public:
    static const InterfaceID &iid () {
        return SID2IID<0xc4438750, 0x06ec, 0x11db, 0x9c, 0xd8, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66>();
    }

    virtual void mute   (uint_least8_t num, bool enable) = 0;
    virtual void volume (uint_least8_t num, uint_least8_t level) = 0;
    virtual void gain   (int_least8_t precent) = 0;
};

class ISidBuilder: public IInterface
{
public:
    static const InterfaceID &iid () {
        return SID2IID<0x1c9ea475, 0xac10, 0x4345, 0x8b, 0x88, 0x3e, 0x48, 0x04, 0xe0, 0xea, 0x38>();
    }

    virtual operator    bool     () const = 0;
    virtual const char *credits  (void) = 0;
    virtual const char *error    (void) const = 0;
    virtual IInterface *lock     (c64env *env, sid2_model_t model) = 0;
    virtual const char *name     (void) const = 0;
    virtual void        unlock   (IInterface *device) = 0;
};

#endif // _sidbuilder_h_
