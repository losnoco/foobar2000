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
** File: sercon.cpp
** License: see jnetlib.h
*/

#include "netinc.h"
#include "util.h"
#include "sercon.h"


JNL_SerCon::JNL_SerCon(int sendbufsize, int recvbufsize)
{
  m_errorstr="";
  m_recv_buffer_len=recvbufsize;
  m_send_buffer_len=sendbufsize;
  m_recv_buffer=(char*)malloc(m_recv_buffer_len);
  m_send_buffer=(char*)malloc(m_send_buffer_len);
  m_file=-1;
  memset(m_recv_buffer,0,recvbufsize);
  memset(m_send_buffer,0,sendbufsize);
  m_recv_len=m_recv_pos=0;
  m_send_len=m_send_pos=0;
  m_err=1; // 1 = not connected
}

void JNL_SerCon::connect(char *dev)
{
  close();
  m_file = open(dev,O_RDWR);
  if (m_file < 0)
  {
    m_err=2;
    m_errorstr = "error opening device";
  }
  else 
  {
    SET_SOCK_BLOCK(m_file,0);
    m_err=0;
  }
}

JNL_SerCon::~JNL_SerCon()
{
  if (m_file >= 0)
  {
    ::close(m_file);
    m_file=-1;
  }
  free(m_recv_buffer);
  free(m_send_buffer);
}

void JNL_SerCon::run(int max_send_bytes, int max_recv_bytes, int *bytes_sent, int *bytes_rcvd)
{
  int bytes_allowed_to_send=(max_send_bytes<0)?m_send_buffer_len:max_send_bytes;
  int bytes_allowed_to_recv=(max_recv_bytes<0)?m_recv_buffer_len:max_recv_bytes;

  if (bytes_sent) *bytes_sent=0;
  if (bytes_rcvd) *bytes_rcvd=0;

  if (m_err) return;

  if (m_send_len>0 && bytes_allowed_to_send>0)
  {
    int len=m_send_buffer_len-m_send_pos;
    if (len > m_send_len) len=m_send_len;
    if (len > bytes_allowed_to_send) len=bytes_allowed_to_send;
    if (len > 0)
    {
      int res=::write(m_file,m_send_buffer+m_send_pos,len);
      if (res==-1 && ERRNO != EWOULDBLOCK)
      {            
        m_err=2;
        m_errorstr = "write() returned error";
        return;
      }
      if (res>0)
      {
        bytes_allowed_to_send-=res;
        if (bytes_sent) *bytes_sent+=res;
        m_send_pos+=res;
        m_send_len-=res;
      }
    }
    if (m_send_pos>=m_send_buffer_len) 
    {
      m_send_pos=0;
      if (m_send_len>0)
      {
        len=m_send_buffer_len-m_send_pos;
        if (len > m_send_len) len=m_send_len;
        if (len > bytes_allowed_to_send) len=bytes_allowed_to_send;
        int res=::write(m_file,m_send_buffer+m_send_pos,len);
        if (res==-1 && ERRNO != EWOULDBLOCK)
        {
          m_err=2;
          m_errorstr = "write() returned error";
          return;
        }
        if (res>0)
        {
          bytes_allowed_to_send-=res;
          if (bytes_sent) *bytes_sent+=res;
          m_send_pos+=res;
          m_send_len-=res;
        }
      }
    }
  }
  if (m_recv_len<m_recv_buffer_len)
  {
    int len=m_recv_buffer_len-m_recv_pos;
    if (len > m_recv_buffer_len-m_recv_len) len=m_recv_buffer_len-m_recv_len;
    if (len > bytes_allowed_to_recv) len=bytes_allowed_to_recv;
    if (len>0)
    {
      int res=::read(m_file,m_recv_buffer+m_recv_pos,len);
      if (res == 0 || (res < 0 && ERRNO != EWOULDBLOCK))
      {        
        m_err=2;
        m_errorstr = "read() returned error";
        return;
      }
      if (res > 0)
      {
        bytes_allowed_to_recv-=res;
        if (bytes_rcvd) *bytes_rcvd+=res;
        m_recv_pos+=res;
        m_recv_len+=res;
      }
    }
    if (m_recv_pos >= m_recv_buffer_len)
    {
       m_recv_pos=0;
       if (m_recv_len < m_recv_buffer_len)
       {
         len=m_recv_buffer_len-m_recv_len;
         if (len > bytes_allowed_to_recv) len=bytes_allowed_to_recv;
         if (len > 0)
         {
           int res=::read(m_file,m_recv_buffer+m_recv_pos,len);
           if (res == 0 || (res < 0 && ERRNO != EWOULDBLOCK))
           {        
             m_err=2;
             m_errorstr = "read() returned error";
             return;
           }
           if (res > 0)
           {
             bytes_allowed_to_recv-=res;
             if (bytes_rcvd) *bytes_rcvd+=res;
             m_recv_pos+=res;
             m_recv_len+=res;
           }
         }
       }
    }
  }
}

void JNL_SerCon::close()
{
  if (m_file >= 0)
  {
    ::close(m_file);
  }
  m_file=-1;
  memset(m_recv_buffer,0,m_recv_buffer_len);
  memset(m_send_buffer,0,m_send_buffer_len);
  m_recv_len=m_recv_pos=0;
  m_send_len=m_send_pos=0;
}

int JNL_SerCon::send_bytes_in_queue(void)
{
  return m_send_len;
}

int JNL_SerCon::send_bytes_available(void)
{
  return m_send_buffer_len-m_send_len;
}

int JNL_SerCon::send(char *data, int length)
{
  if (length > send_bytes_available())
  {
    return -1;
  }
  
  int write_pos=m_send_pos+m_send_len;
  if (write_pos >= m_send_buffer_len) 
  {
    write_pos-=m_send_buffer_len;
  }

  int len=m_send_buffer_len-write_pos;
  if (len > length) 
  {
    len=length;
  }

  memcpy(m_send_buffer+write_pos,data,len);
  if (length > len)
  {
    memcpy(m_send_buffer,data+len,length-len);
  }
  m_send_len+=length;
  return 0;
}

int JNL_SerCon::send_string(char *line)
{
  return send(line,strlen(line));
}

int JNL_SerCon::recv_bytes_available(void)
{
  return m_recv_len;
}

int JNL_SerCon::peek_bytes(char *data, int maxlength)
{
  if (maxlength > m_recv_len)
  {
    maxlength=m_recv_len;
  }
  int read_pos=m_recv_pos-m_recv_len;
  if (read_pos < 0) 
  {
    read_pos += m_recv_buffer_len;
  }
  int len=m_recv_buffer_len-read_pos;
  if (len > maxlength)
  {
    len=maxlength;
  }
  memcpy(data,m_recv_buffer+read_pos,len);
  if (len < maxlength)
  {
    memcpy(data+len,m_recv_buffer,maxlength-len);
  }

  return maxlength;
}

int JNL_SerCon::recv_bytes(char *data, int maxlength)
{
  
  int ml=peek_bytes(data,maxlength);
  m_recv_len-=ml;
  return ml;
}

int JNL_SerCon::getbfromrecv(int pos, int remove)
{
  int read_pos=m_recv_pos-m_recv_len + pos;
  if (pos < 0 || pos > m_recv_len) return -1;
  if (read_pos < 0) 
  {
    read_pos += m_recv_buffer_len;
  }
  if (read_pos >= m_recv_buffer_len)
  {
    read_pos-=m_recv_buffer_len;
  }
  if (remove) m_recv_len--;
  return m_recv_buffer[read_pos];
}

int JNL_SerCon::recv_lines_available(void)
{
  int l=recv_bytes_available();
  int lcount=0;
  int lastch=0;
  int pos;
  for (pos=0; pos < l; pos ++)
  {
    int t=getbfromrecv(pos,0);
    if (t == -1) return lcount;
    if ((t=='\r' || t=='\n') &&(
         (lastch != '\r' && lastch != '\n') || lastch==t
        )) lcount++;
    lastch=t;
  }
  return lcount;
}

int JNL_SerCon::recv_line(char *line, int maxlength)
{
  if (maxlength > m_recv_len) maxlength=m_recv_len;
  while (maxlength--)
  {
    int t=getbfromrecv(0,1);
    if (t == -1) 
    {
      *line=0;
      return 0;
    }
    if (t == '\r' || t == '\n')
    {
      int r=getbfromrecv(0,0);
      if ((r == '\r' || r == '\n') && r != t) getbfromrecv(0,1);
      *line=0;
      return 0;
    }
    *line++=(char)t;
  }
  return 1;
}
