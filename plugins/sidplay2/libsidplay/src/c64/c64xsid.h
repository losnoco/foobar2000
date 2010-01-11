/***************************************************************************
                          c64sid.h  -  ReSid Wrapper
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

/* This file could be a specialisation of a sid implementation.
 * However since the sid emulation is not part of this project
 * we are actually creating a wrapper instead.
 */

#include "c64env.h"
#include "sidconfig.h"
#include "sidbuilder.h"
#include "sidlazyiptr.h"
#include "imp/sidcoaggregate.h"
#include "../xsid/xsid.h"

SIDPLAY2_NAMESPACE_START

class c64xsid: public XSID, CoAggregate<ISidMixer>
{
private:
    c64env                    &m_env;
    SidLazyIPtr<ISidEmulation> m_sid;
    SidLazyIPtr<ISidMixer>     m_mixer;
    int_least32_t              m_gain;

private:
    uint8_t readMemByte (uint_least16_t addr)
    {
        uint8_t data = m_env.readMemRamByte (addr);
        m_env.sid2crc (data);
        return data;
    }

    void writeMemByte (uint8_t data) { m_sid->write (0x18, data); }

    bool _iquery (const Iid &iid, void **implementation)
    {
        if (iid == ISidMixer::iid())
        {
            *implementation = static_cast<ISidMixer *>(this);
            return true;
        }
        return XSID::_iquery (iid, implementation);
    }

public:
    c64xsid (c64env *env)
    :XSID(&env->context ()),
     CoAggregate<ISidMixer>(*iaggregate()),
     m_env(*env), m_sid(0), m_mixer(0), m_gain(100)
    {
        ;
    }

    ISidUnknown *iaggregate () { return XSID::iaggregate (); }

    // Standard component interface
    const char *error (void) {return "";}
    void reset (uint8_t volume)
    {
        XSID::reset  (volume);
        m_sid->reset (volume);
    }

    uint8_t read (uint_least8_t addr)
    {   return m_sid->read (addr); }

    void write (uint_least8_t addr, uint8_t data)
    {
        if (addr == 0x18)
            XSID::storeSidData0x18 (data);
        else
            m_sid->write (addr, data);
    }

    void write16 (uint_least16_t addr, uint8_t data)
    {
        XSID::write (addr, data);
    }

    // Standard SID interface
    int_least32_t output (uint_least8_t bits)
    {
        return m_sid->output (bits) + (XSID::output (bits) * m_gain / 100);
    }

    void volume (uint_least8_t num, uint_least8_t vol)
    {
        if (m_mixer)
            m_mixer->volume (num, vol);
    }

    void mute (uint_least8_t num, bool mute)
    {
        if (num == 3)
            XSID::mute  (mute);
        else if (m_mixer)
            m_mixer->mute (num, mute);
    }

    void mute (bool mute) { XSID::mute (mute); }

    void gain (int_least8_t percent)
    {
        // 0 to 99 is loss, 101 - 200 is gain
        m_gain  = percent;
        m_gain += 100;
        if (m_gain > 200)
            m_gain = 200;
    }

    // Xsid specific
    void emulation (SidIPtr<ISidEmulation> &sid)
    {
        m_mixer = sid;
        m_sid   = sid;
    }

    ISidUnknown *emulation (void) { return &m_sid; }
};

SIDPLAY2_NAMESPACE_STOP
