/***************************************************************************
                          sidconfig.h  -  Redirect to real sidconfig.h
                             -------------------
    begin                : Tues Dec 4 2001
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
 *  $Log: sidconfig.h,v $
 *  Revision 1.6  2008/02/27 20:59:27  s_a_white
 *  Re-sync COM like interface and update to final names.
 *
 *  Revision 1.5  2001/12/07 00:40:22  s_a_white
 *  Windows fixes.
 *
 *  Revision 1.4  2001/12/05 23:37:39  s_a_white
 *  Now redirects to real sidfig.hy/i
 *
 ***************************************************************************/

#if defined(HAVE_UNIX)
#   include "../../unix/sidconfig.h"
#elif defined(HAVE_MSWINDOWS)
#   ifdef _MSC_VER
#       if _MSC_VER < 1300
#           include "../../win/VC5/sidconfig.h"
#       else
#           include "../../win/VC/sidconfig.h"
#       endif
#   else
#       error Compiler not supported!
#   endif
#else
#   error Platform not supported!
#endif
