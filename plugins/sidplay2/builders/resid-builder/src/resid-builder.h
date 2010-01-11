/***************************************************************************
                          resid.h  -  ReSid Builder
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

#ifndef _resid_builder_h_
#define _resid_builder_h_

/* Since ReSID is not part of this project we are actually
 * creating a wrapper instead of implementing a SID emulation
 */

#include <vector>
#include <sidplay/imp/sidbuilder.h>
#include <sidplay/event.h>
#include "resid.h"

class ReSID;

/***************************************************************************
 * ReSID Builder Class
 ***************************************************************************/
// Create the SID builder object
class ReSIDBuilderImpl: public SidBuilder<ReSIDBuilder>
{
protected:
    std::vector<ReSID *> sidobjs;

private:
    static const char  *ERR_FILTER_DEFINITION;
    char        m_errorBuffer[100];
    const char *m_error;

public:
    ReSIDBuilderImpl  (const char * name);
    ~ReSIDBuilderImpl (void);

    // IInterface
    bool ifquery (const InterfaceID &iid, void **implementation);

    // true will give you the number of used devices.
    //    return values: 0 none, positive is used sids
    // false will give you all available sids.
    //    return values: 0 endless, positive is available sids.
    // use bool operator to determine error
    uint        devices (bool used);
    uint        create  (uint sids);
    IInterface *lock    (c64env *env, sid2_model_t model);
    void        unlock  (IInterface *device);
    void        remove  (void);
    const char *error   (void) const { return m_error; }
    const char *credits (void);

    // Settings that effect all SIDs
    void filter   (bool enable);
    void filter   (const sid_filter_t *filter);
    void sampling (uint_least32_t freq);
};

#endif // _resid_builder_h_
