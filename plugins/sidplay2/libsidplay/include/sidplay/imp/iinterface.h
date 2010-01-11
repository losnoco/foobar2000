/***************************************************************************
                          imp_component.h  -  Component Implementation
                             -------------------
    begin                : Sat Jun 6 2006
    copyright            : (C) 2006 by Simon White
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

#ifndef _imp_ifbase_h_
#define _imp_ifbase_h_

// This file is only to be used be things creating implementations.
// DO NOT include this file in external code usings these implementations,
// use interface files instead:

#include <string.h>
#include <sidplay/iinterface.h>

template <class TImplementation>
class ICoInterface: public TImplementation
{
private:
    const char * const m_name;
    unsigned int m_refcount;

public:
    ICoInterface (const char *name) : m_name(name), m_refcount(0) { ; }
    virtual ~ICoInterface () { ; }

    virtual const InterfaceID &ifid () const { return TImplementation::iid(); 
} 
    IInterface *aggregate () { return this; }
    const char *name () const { return m_name; }

protected:
    virtual void _ifdestroy () { ; }

private:
    unsigned int ifadd     () { return ++m_refcount; }
    unsigned int ifrelease () { if (!--m_refcount) _ifdestroy (); return m_refcount; }
};

template <class TImplementation>
class ICoAggregate: public TImplementation
{
private:
    IInterface &m_unknown;

public:
    ICoAggregate (IInterface &unknown)
        : m_unknown(unknown) { ; }

private:
    const InterfaceID &ifid () const { return m_unknown.ifid (); }

    IInterface *aggregate ()  { return m_unknown.aggregate (); }
    const char *name () const { return m_unknown.name (); }

    unsigned int ifadd     () { return m_unknown.ifadd (); }
    bool         ifquery   (const InterfaceID &iid, void **implementation)
                              { return m_unknown.ifquery (iid, implementation); }
    unsigned int ifrelease () { return m_unknown.ifrelease (); }
};

#endif // _imp_ifbase_h_
