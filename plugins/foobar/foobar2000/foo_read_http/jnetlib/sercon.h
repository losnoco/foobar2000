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
** Copyright (C) 2000-2001 Nullsoft, Inc.
** Author: Justin Frankel
** File: sercon.h
** License: see jnetlib.h
**
*/

#ifndef _SERCON_H_
#define _SERCON_H_

class JNL_SerCon
{
  public:

    JNL_SerCon(int sendbufsize=8192, int recvbufsize=8192);
    ~JNL_SerCon();

    void connect(char *device);

    void run(int max_send_bytes=-1, int max_recv_bytes=-1, int *bytes_sent=NULL, int *bytes_rcvd=NULL);
    int get_error() { return m_err; } // 1 on error, 2 on no connection, 0 on connected
    char *get_errstr() { return m_errorstr; }

    void close();
    void flush_send(void) { m_send_len=m_send_pos=0; }

    int send_bytes_in_queue(void);
    int send_bytes_available(void);
    int send(char *data, int length); // returns -1 if not enough room
    int send_string(char *line);      // returns -1 if not enough room

    int recv_bytes_available(void);
    int recv_bytes(char *data, int maxlength); // returns actual bytes read
    unsigned int recv_int(void);
    int recv_lines_available(void);
    int recv_line(char *line, int maxlength); // returns 0 if the line was terminated with a \r or \n, 1 if not.
                                              // (i.e. if you specify maxlength=10, and the line is 12 bytes long
                                              // it will return 1. or if there is no \r or \n and that's all the data
                                              // the connection has.)
    int peek_bytes(char *data, int maxlength); // returns bytes peeked

  
  protected:
    int  m_file;
    char *m_recv_buffer;
    char *m_send_buffer;
    int m_recv_buffer_len;
    int m_send_buffer_len;

    int  m_recv_pos;
    int  m_recv_len;
    int  m_send_pos;
    int  m_send_len;

    int m_err;
    char *m_errorstr;

    int getbfromrecv(int pos, int remove); // used by recv_line*

};

#endif // _SERCON_H_
