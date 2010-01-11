/***************************************************************************
         resid-builder.cpp - ReSID builder class for creating/controlling
                             resids.
                             -------------------
    begin                : Wed Sep 5 2001
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
/***************************************************************************
 *  $Log: resid-builder.cpp,v $
 *  Revision 1.18  2008/03/02 22:50:30  s_a_white
 *  Create calls should be in global namespace
 *
 *  Revision 1.17  2008/02/27 20:58:52  s_a_white
 *  Re-sync COM like interface and update to final names.
 *
 *  Revision 1.16  2007/01/27 10:21:39  s_a_white
 *  Updated to use better COM emulation interface.
 *
 *  Revision 1.15  2006/10/28 09:09:16  s_a_white
 *  Update to newer COM style interface
 *
 *  Revision 1.14  2006/10/20 16:30:41  s_a_white
 *  Linker fix
 *
 *  Revision 1.13  2006/10/20 16:29:32  s_a_white
 *  Build fix
 *
 *  Revision 1.12  2006/10/20 16:23:21  s_a_white
 *  Improve compatibility with old code.
 *
 *  Revision 1.11  2006/06/29 19:12:46  s_a_white
 *  Seperate mixer interface from emulation interface.
 *
 *  Revision 1.10  2006/06/27 19:44:36  s_a_white
 *  Add return parameter to ifquery.
 *
 *  Revision 1.9  2006/06/27 19:19:54  s_a_white
 *  Create entry point for builder creation (eventually this will be a module)
 *
 *  Revision 1.8  2006/06/21 20:04:06  s_a_white
 *  Add ifquery function
 *
 *  Revision 1.7  2006/06/21 19:58:19  s_a_white
 *  Convert over to COM style interface.
 *
 *  Revision 1.6  2002/10/17 18:45:31  s_a_white
 *  Exit unlock function early once correct sid is found.
 *
 *  Revision 1.5  2002/03/04 19:06:38  s_a_white
 *  Fix C++ use of nothrow.
 *
 *  Revision 1.4  2002/01/29 21:58:28  s_a_white
 *  Moved out sid emulation to a private header file.
 *
 *  Revision 1.3  2001/12/11 19:33:18  s_a_white
 *  More GCC3 Fixes.
 *
 *  Revision 1.2  2001/12/09 10:53:50  s_a_white
 *  Added exporting of credits.
 *
 *  Revision 1.1.1.1  2001/11/25 15:03:20  s_a_white
 *  Initial Release
 *
 ***************************************************************************/

#include <stdio.h>

#include "config.h"
#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif

#include "resid-builder.h"
#include "resid-emu.h"

SIDPLAY2_NAMESPACE_START

// Error String(s)
const char *CoReSIDBuilder::ERR_FILTER_DEFINITION = "RESID ERROR: Filter definition is not valid (see docs).";

CoReSIDBuilder::CoReSIDBuilder (const char * name)
:CoBuilder<IReSIDBuilder>(name)
{
    m_error = "N/A";
}

CoReSIDBuilder::~CoReSIDBuilder (void)
{   // Remove all are SID emulations
    remove ();
}

// Create a new sid emulation.  Called by libsidplay2 only
uint CoReSIDBuilder::create (uint sids)
{
    uint   count;
    ReSID *sid = NULL;
    m_status   = true;

    // Check available devices
    count = devices (false);
    if (!m_status)
        goto CoReSIDBuilder_create_error;
    if (count && (count < sids))
        sids = count;

    for (count = 0; count < sids; count++)
    {
#   ifdef HAVE_EXCEPTIONS
        sid = new(std::nothrow) ReSID(this);
#   else
        sid = new ReSID(this);
#   endif

        // Memory alloc failed?
        if (!sid)
        {
            sprintf (m_errorBuffer, "%s ERROR: Unable to create ReSID object", iname ());
            m_error = m_errorBuffer;
            goto CoReSIDBuilder_create_error;
        }

        // SID init failed?
        if (!*sid)
        {
            m_error = sid->error ();
            goto CoReSIDBuilder_create_error;
        }

        sidobjs.push_back (sid);
    }
    return count;

CoReSIDBuilder_create_error:
    m_status = false;
    if (sid)
        delete sid;
    return count;
}

const char *CoReSIDBuilder::credits ()
{
    m_status = true;

    // Available devices
    if (sidobjs.size ())
    {
        ReSID *sid = sidobjs[0];
        return sid->credits ();
    }

    {   // Create an emulation to obtain credits
        ReSID sid(this);
        if (!sid)
        {
            m_status = false;
            strcpy (m_errorBuffer, sid.error ());
            return 0;
        }
        return sid.credits ();
    }
}


uint CoReSIDBuilder::devices (bool created)
{
    m_status = true;
    if (created)
        return sidobjs.size ();
    else // Available devices
        return 0;
}

void CoReSIDBuilder::filter (const sid_filter_t *filter)
{
    int size = sidobjs.size ();
    m_status = true;
    for (int i = 0; i < size; i++)
    {
        ReSID *sid = sidobjs[i];
        if (!sid->filter (filter))
            goto CoReSIDBuilder_sidFilterDef_error;
    }
return;

CoReSIDBuilder_sidFilterDef_error:
    m_error  = ERR_FILTER_DEFINITION;
    m_status = false;
}

void CoReSIDBuilder::filter (bool enable)
{
    int size = sidobjs.size ();
    m_status = true;
    for (int i = 0; i < size; i++)
    {
        ReSID *sid = sidobjs[i];
        sid->filter (enable);
    }
}

// Find a free SID of the required specs
ISidUnknown *CoReSIDBuilder::lock (c64env *env, sid2_model_t model)
{
    int size = sidobjs.size ();
    m_status = true;

    for (int i = 0; i < size; i++)
    {
        ReSID *sid = sidobjs[i];
        if (sid->lock (env))
        {
            sid->model (model);
            return sid->iaggregate ();
        }
    }
    // Unable to locate free SID
    m_status = false;
    sprintf (m_errorBuffer, "%s ERROR: No available SIDs to lock", iname ());
    return NULL;
}

// Allow something to use this SID
void CoReSIDBuilder::unlock (ISidUnknown &device)
{
    ISidUnknown *emulation = device.iaggregate ();
    int size = sidobjs.size ();
    // Maek sure this is our SID
    for (int i = 0; i < size; i++)
    {
        if (sidobjs[i]->iaggregate() == emulation)
        {   // Unlock it
            ReSID *sid = sidobjs[i];
            sid->lock (NULL);
            break;
        }
    }
}

// Remove all SID emulations.
void CoReSIDBuilder::remove ()
{
    sidobjs.clear();
}

void CoReSIDBuilder::sampling (uint_least32_t freq)
{
    int size = sidobjs.size ();
    m_status = true;
    for (int i = 0; i < size; i++)
    {
        ReSID *sid = sidobjs[i];
        sid->sampling (freq);
    }
}

// Find the correct interface
bool CoReSIDBuilder::_iquery (const Iid &iid, void **implementation)
{
    if (iid == IReSIDBuilder::iid())
        *implementation = static_cast<IReSIDBuilder *>(this);
    else if (iid == ISidBuilder::iid())
        *implementation = static_cast<IReSIDBuilder *>(this);
    else if (iid == ISidUnknown::iid())
        *implementation = static_cast<IReSIDBuilder *>(this);
    else
        return false;
    return true;
}

SIDPLAY2_NAMESPACE_STOP

using SIDPLAY2_NAMESPACE::CoReSIDBuilder;

// Entry point
ISidUnknown *ReSIDBuilderCreate (const char * const name)
{
#ifdef HAVE_EXCEPTIONS
	CoReSIDBuilder *builder = new(std::nothrow) CoReSIDBuilder(name);
#else
    CoReSIDBuilder *builder = new CoReSIDBuilder(name);
#endif
    if (builder)
        return builder->iaggregate ();
    return 0;
}
