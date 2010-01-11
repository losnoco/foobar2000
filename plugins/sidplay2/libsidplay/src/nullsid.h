/***************************************************************************
                          nullsid.h  -  Null SID Emulation
                             -------------------
    begin                : Thurs Sep 20 2001
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

#ifndef _nullsid_h_
#define _nullsid_h_

#include "sidconfig.h"
#include "sidbuilder.h"

SIDPLAY2_NAMESPACE_START

class NullSID: public CoEmulation<ISidEmulation>,
               public CoAggregate<ISidMixer>
{
private:
    bool _iquery (const Iid &iid, void **implementation)
    {
        if (iid == ISidEmulation::iid())
            *implementation = static_cast<ISidEmulation *>(this);
        else if (iid == ISidMixer::iid())
            *implementation = static_cast<ISidMixer *>(this);
        else if (iid == ISidUnknown::iid())
            *implementation = static_cast<ISidEmulation *>(this);
        else
            return false;
        return true;
    }

public:
    NullSID ()
    :CoEmulation<ISidEmulation>("NullSID", NULL),
     CoAggregate<ISidMixer>(*iaggregate()) {;}

    ISidUnknown *iaggregate () { return CoEmulation<ISidEmulation>::iaggregate (); }

    // Standard component functions
    void    reset (uint8_t) { ; }
    uint8_t read  (uint_least8_t) { return 0; }
    void    write (uint_least8_t, uint8_t) { ; }
    const   char *credits (void) { return ""; }
    const   char *error   (void) { return ""; }

    // Standard SID functions
    int_least32_t output (uint_least8_t) { return 0; }
    void          volume (uint_least8_t, uint_least8_t) { ; }
    void          mute   (uint_least8_t, bool) { ; }
    void          gain   (int_least8_t) { ; }
};

SIDPLAY2_NAMESPACE_STOP

#endif // _nullsid_h_
