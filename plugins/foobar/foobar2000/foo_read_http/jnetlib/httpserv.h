/*

  Nullsoft WASABI Source File License

  Copyright 1999-2001 Nullsoft, Inc.

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.


  Brennan Underwood
  brennan@nullsoft.com

*/

/*
** JNetLib
** Copyright (C) 2001 Nullsoft, Inc.
** Author: Justin Frankel
** File: httpserv.h - JNL interface for doing HTTP GET/POST serving.
** License: see jnetlib.h
** This class just manages the http reply/sending, not where the data 
** comes from, etc.
*/

#ifndef _HTTPSERV_H_
#define _HTTPSERV_H_

#include "connection.h"

class JNL_HTTPServ
{
  public:
    JNL_HTTPServ(JNL_Connection *con);
    ~JNL_HTTPServ();

    int run(); // returns: < 0 on error, 0 on request not read yet, 1 if reading headers, 2 if reply not sent, 3 if reply sent, sending data. 4 on connection closed.

    char *geterrorstr() { return m_errstr;}

    // use these when state returned by run() is 2 
    char *get_request_file(); // file portion of http request
    char *get_request_parm(char *parmname); // parameter portion (after ?)
    char *getallheaders() { return m_recvheaders; } // double null terminated, null delimited list
    char *getheader(char *headername);

    void set_reply_string(char *reply_string); // should be HTTP/1.1 OK or the like
    void set_reply_header(char *header); // i.e. "content-size: 12345"

    void send_reply() { m_reply_ready=1; } // send reply, state will advance to 3.

    ////////// sending data ///////////////
    int bytes_inqueue() { if (m_state == 3 || m_state == -1 || m_state ==4) return m_con->send_bytes_in_queue(); else return 0; }
    int bytes_cansend() { if (m_state == 3) return m_con->send_bytes_available(); else return 0; }
    void write_bytes(char *bytes, int length) { m_con->send(bytes,length); }

    void close(int quick) { m_con->close(quick); m_state=4; }

    JNL_Connection *get_con() { return m_con; }

  protected:
    void seterrstr(char *str) { if (m_errstr) free(m_errstr); m_errstr=(char*)malloc(strlen(str)+1); strcpy(m_errstr,str); }

    int m_reply_ready;
    int m_state;

    char *m_errstr;
    char *m_reply_headers;
    char *m_reply_string;
    char *m_recvheaders;
    int   m_recvheaders_size;
    char *m_recv_request; // either double-null terminated, or may contain parameters after first null.
    JNL_Connection *m_con;
};

#endif // _HTTPSERV_H_
