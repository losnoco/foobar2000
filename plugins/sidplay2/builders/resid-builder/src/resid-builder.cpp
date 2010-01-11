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

// Error String(s)
const char *ReSIDBuilderImpl::ERR_FILTER_DEFINITION = "RESID ERROR: Filter definition is not valid (see docs).";

ReSIDBuilderImpl::ReSIDBuilderImpl (const char * name)
:SidBuilder<ReSIDBuilder>(name)
{
    m_error = "N/A";
}

ReSIDBuilderImpl::~ReSIDBuilderImpl (void)
{   // Remove all are SID emulations
    remove ();
}

// Create a new sid emulation.  Called by libsidplay2 only
uint ReSIDBuilderImpl::create (uint sids)
{
    uint   count;
    ReSID *sid = NULL;
    m_status   = true;

    // Check available devices
    count = devices (false);
    if (!m_status)
        goto ReSIDBuilderImpl_create_error;
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
            sprintf (m_errorBuffer, "%s ERROR: Unable to create ReSID object", name ());
            m_error = m_errorBuffer;
            goto ReSIDBuilderImpl_create_error;
        }

        // SID init failed?
        if (!*sid)
        {
            m_error = sid->error ();
            goto ReSIDBuilderImpl_create_error;
        }

        sidobjs.push_back (sid);
    }
    return count;

ReSIDBuilderImpl_create_error:
    m_status = false;
    if (sid)
        delete sid;
    return count;
}

const char *ReSIDBuilderImpl::credits ()
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


uint ReSIDBuilderImpl::devices (bool created)
{
    m_status = true;
    if (created)
        return sidobjs.size ();
    else // Available devices
        return 0;
}

void ReSIDBuilderImpl::filter (const sid_filter_t *filter)
{
    int size = sidobjs.size ();
    m_status = true;
    for (int i = 0; i < size; i++)
    {
        ReSID *sid = sidobjs[i];
        if (!sid->filter (filter))
            goto ReSIDBuilderImpl_sidFilterDef_error;
    }
return;

ReSIDBuilderImpl_sidFilterDef_error:
    m_error  = ERR_FILTER_DEFINITION;
    m_status = false;
}

void ReSIDBuilderImpl::filter (bool enable)
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
IInterface *ReSIDBuilderImpl::lock (c64env *env, sid2_model_t model)
{
    int size = sidobjs.size ();
    m_status = true;

    for (int i = 0; i < size; i++)
    {
        ReSID *sid = sidobjs[i];
        if (sid->lock (env))
        {
            sid->model (model);
            return sid->aggregate ();
        }
    }
    // Unable to locate free SID
    m_status = false;
    sprintf (m_errorBuffer, "%s ERROR: No available SIDs to lock", name ());
    return NULL;
}

// Allow something to use this SID
void ReSIDBuilderImpl::unlock (IInterface *device)
{
    IInterface *emulation = device->aggregate ();
    int size = sidobjs.size ();
    // Maek sure this is our SID
    for (int i = 0; i < size; i++)
    {
        if (sidobjs[i]->aggregate() == emulation)
        {   // Unlock it
            ReSID *sid = sidobjs[i];
            sid->lock (NULL);
            break;
        }
    }
}

// Remove all SID emulations.
void ReSIDBuilderImpl::remove ()
{
    sidobjs.clear();
}

void ReSIDBuilderImpl::sampling (uint_least32_t freq)
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
bool ReSIDBuilderImpl::ifquery (const InterfaceID &iid, void **implementation)
{
    if (iid == ReSIDBuilder::iid())
        *implementation = static_cast<ReSIDBuilder *>(this);
    else if (iid == ISidBuilder::iid())
        *implementation = static_cast<ReSIDBuilder *>(this);
    else if (iid == IInterface::iid())
        *implementation = static_cast<ReSIDBuilder *>(this);
    else
        return false;
    return true;
}


// Entry point
IInterface *ReSIDBuilderCreate (const char * const name)
{
#ifdef HAVE_EXCEPTIONS
	ReSIDBuilderImpl *builder = new(std::nothrow) ReSIDBuilderImpl(name);
#else
    ReSIDBuilderImpl *builder = new ReSIDBuilderImpl(name);
#endif
    if (builder)
        return builder->aggregate ();
    return 0;
}
