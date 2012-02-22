/***************************************************************************
                          player.h  -  description
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


#ifndef _player_h_
#define _player_h_

#include "sid2types.h"
#include "SidTune.h"

#include "mixer.h"

#include "c64/c64.h"

SIDPLAY2_NAMESPACE_START

class Player
{
private:
    static const char  TXT_PAL_VBI[];
    static const char  TXT_PAL_VBI_FIXED[];
    static const char  TXT_PAL_CIA[];
    static const char  TXT_PAL_UNKNOWN[];
    static const char  TXT_NTSC_VBI[];
    static const char  TXT_NTSC_VBI_FIXED[];
    static const char  TXT_NTSC_CIA[];
    static const char  TXT_NTSC_UNKNOWN[];
    static const char  TXT_NA[];

    static const char  ERR_UNSUPPORTED_FREQ[];
    static const char  ERR_UNSUPPORTED_PRECISION[];
    static const char  ERR_UNKNOWN_ROM[];
    static const char  ERR_PSIDDRV_NO_SPACE[];
    static const char  ERR_PSIDDRV_RELOC[];

    static const char  *credit[10]; // 10 credits max

    c64     m_c64;

    Mixer   m_mixer;

    // User Configuration Settings
    SidTuneInfo   m_tuneInfo;
    SidTune      *m_tune;
    sid2_info_t   m_info;
    sid2_config_t m_cfg;

    const char     *m_errorString;
    uint_least32_t  m_mileage;
    volatile sid2_player_t m_playerState;
    int             m_rand;

    bool            m_status;

private:
    float64_t clockSpeed     (sid2_clock_t clock, sid2_clock_t defaultClock,
                              const bool forced);
    int       initialise     (void);
    int       sidCreate(sidbuilder *builder, const sid2_model_t userModel,
                       const sid2_model_t defaultModel, const int channels,
                       const float64_t cpuFreq, const int frequency,
                       const sampling_method_t sampling);
    uint8_t   iomap          (const uint_least16_t addr);
    sid2_model_t getModel(const int sidModel,
                          sid2_model_t userModel,
                          sid2_model_t defaultModel);

    uint16_t getChecksum(const uint8_t* rom, const int size);

    // Rev 2.0.3 Added - New Mixer Routines
    uint_least32_t (Player::*output) (char *buffer);

    // PSID driver
    int  psidDrvReloc   (SidTuneInfo &tuneInfo, sid2_info_t &info);

public:
    Player ();
    ~Player () {}

    const sid2_config_t &config (void) const { return m_cfg; }
    const sid2_info_t   &info   (void) const { return m_info; }

    int            config       (const sid2_config_t &cfg);
    int            fastForward  (uint percent);
    int            load         (SidTune *tune);
    uint_least32_t mileage      (void) { return m_mileage + time(); }
    float64_t      cpuFreq      (void) const { return m_c64.getMainCpuSpeed(); }
    uint_least32_t play         (short *buffer, uint_least32_t samples);
    sid2_player_t  state        (void) const { return m_playerState; }
    void           stop         (void);
    uint_least32_t time         (void) { return (uint_least32_t)(m_c64.getEventScheduler()->getTime(EVENT_CLOCK_PHI1) / cpuFreq()); }
    void           debug        (const bool enable, FILE *out)
                                { m_c64.debug (enable, out); }
    void           mute         (const int voice, const bool enable);

    const char    *error        (void) const { return m_errorString; }
    bool           getStatus() const { return m_status; }

    void setRoms(const uint8_t* kernal, const uint8_t* basic, const uint8_t* character);

    SidTuneInfo *getTuneInfo() { return m_tune ? &m_tuneInfo : 0; }

    EventContext *getEventScheduler() {return m_c64.getEventScheduler(); }

};

SIDPLAY2_NAMESPACE_STOP

#endif // _player_h_