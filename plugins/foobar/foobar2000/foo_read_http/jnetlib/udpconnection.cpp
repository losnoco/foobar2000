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
** File: udpconnection.cpp - JNL UDP connection implementation
** License: see jnetlib.h
*/

#include "netinc.h"
#include "util.h"
#include "udpconnection.h"


JNL_UDPConnection::JNL_UDPConnection(short incoming_port, JNL_AsyncDNS *dns, int sendbufsize, int recvbufsize)
{
  m_errorstr="";
  if (dns == JNL_CONNECTION_AUTODNS)
  {
    m_dns=new JNL_AsyncDNS();
    m_dns_owned=1;
  }
  else
  {
    m_dns=dns;
    m_dns_owned=0;
  }
  m_recv_buffer_len=recvbufsize;
  m_send_buffer_len=sendbufsize;
  m_recv_buffer=(char*)malloc(m_recv_buffer_len);
  m_send_buffer=(char*)malloc(m_send_buffer_len);
  m_socket=-1;
  memset(m_recv_buffer,0,recvbufsize);
  memset(m_send_buffer,0,sendbufsize);
  m_remote_port=0;
  m_state=STATE_NOCONNECTION;
  m_recv_len=m_recv_pos=0;
  m_send_len=m_send_pos=0;
  m_host[0]=0;
  memset(&m_saddr,0,sizeof(struct sockaddr_in));
  m_socket=::socket(PF_INET,SOCK_DGRAM,0);
  if (m_socket==-1)
  {
    m_errorstr="creating socket";
    m_state=STATE_ERROR;
  }
  SET_SOCK_BLOCK(m_socket,0);
  memset(&m_iaddr,0,sizeof(struct sockaddr_in));
  m_iaddr.sin_family=AF_INET;
  m_iaddr.sin_port=htons(incoming_port);
  m_iaddr.sin_addr.s_addr = htonl( INADDR_ANY );
  if(::bind(m_socket,(struct sockaddr *)&m_iaddr,sizeof(m_iaddr))==-1)
  {
    m_errorstr="binding socket";
    m_state=STATE_ERROR;
  }
  m_state=STATE_CONNECTED;
}

void JNL_UDPConnection::setpeer(char *hostname, int port)
{
  m_remote_port=(short)port;
  strncpy(m_host,hostname,sizeof(m_host)-1);
  m_host[sizeof(m_host)-1]=0;
  memset(&m_saddr,0,sizeof(m_saddr));
  if (!m_host[0])
  {
    m_errorstr="empty hostname";
    m_state=STATE_ERROR;
  }
  else
  {
    m_state=STATE_RESOLVING;
    m_saddr.sin_family=AF_INET;
    m_saddr.sin_port=htons((unsigned short)port);
    m_saddr.sin_addr.s_addr=inet_addr(hostname);
  }
}

void JNL_UDPConnection::setpeer(struct sockaddr *addr)
{
  memcpy(&m_saddr, (const void *)addr, sizeof(struct sockaddr_in));
}

JNL_UDPConnection::~JNL_UDPConnection()
{
  if (m_socket >= 0)
  {
    ::shutdown(m_socket, SHUT_RDWR);
    ::closesocket(m_socket);
    m_socket=-1;
  }
  free(m_recv_buffer);
  free(m_send_buffer);
  if (m_dns_owned) 
  {
    delete m_dns;
  }
}

void JNL_UDPConnection::run(int max_send_bytes, int max_recv_bytes, int *bytes_sent, int *bytes_rcvd)
{
  int bytes_allowed_to_send=(max_send_bytes<0)?m_send_buffer_len:max_send_bytes;
  int bytes_allowed_to_recv=(max_recv_bytes<0)?m_recv_buffer_len:max_recv_bytes;

  if (bytes_sent) *bytes_sent=0;
  if (bytes_rcvd) *bytes_rcvd=0;

  switch (m_state)
  {
    case STATE_RESOLVING:
      if (m_saddr.sin_addr.s_addr == INADDR_NONE)
      {
        int a=m_dns?m_dns->resolve(m_host,(unsigned long int *)&m_saddr.sin_addr.s_addr):-1;
        if (!a) { m_state=STATE_CONNECTED; }
        else if (a == 1)
        {
          m_state=STATE_RESOLVING; 
          break;
        }
        else
        {
          m_errorstr="resolving hostname"; 
          m_state=STATE_ERROR; 
          return;
        }
      }
      m_state=STATE_CONNECTED;
    break;
    case STATE_CONNECTED:
    case STATE_CLOSING:
      if (m_send_len>0 && bytes_allowed_to_send>0)
      {
        int len=m_send_buffer_len-m_send_pos;
        if (len > m_send_len) len=m_send_len;
        if (len > bytes_allowed_to_send) len=bytes_allowed_to_send;
        if (len > 0)
        {
          int res=::sendto(m_socket,m_send_buffer+m_send_pos,len,0,(struct sockaddr *)&m_saddr,sizeof(struct sockaddr_in));
          if (res==-1 && ERRNO != EWOULDBLOCK)
          {            
//            m_state=STATE_CLOSED;
//            return;
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
            int res=::sendto(m_socket,m_send_buffer+m_send_pos,len,0,(struct sockaddr *)&m_saddr,sizeof(struct sockaddr_in));
            if (res==-1 && ERRNO != EWOULDBLOCK)
            {
//              m_state=STATE_CLOSED;
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
          int lg=sizeof(m_laddr);
          int res=::recvfrom(m_socket,m_recv_buffer+m_recv_pos,len,0,(struct sockaddr *)&m_laddr,(socklen_t *)&lg);
          if (res == 0 || (res < 0 && ERRNO != EWOULDBLOCK))
          {        
            m_state=STATE_CLOSED;
            break;
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
              int lg=sizeof(m_laddr);
              int res=::recvfrom(m_socket,m_recv_buffer+m_recv_pos,len,0,(struct sockaddr *)&m_laddr,(socklen_t *)&lg);
              if (res == 0 || (res < 0 && ERRNO != EWOULDBLOCK))
              {        
                m_state=STATE_CLOSED;
                break;
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
      if (m_state == STATE_CLOSING)
      {
        if (m_send_len < 1) m_state = STATE_CLOSED;
      }
    break;
    default: break;
  }
}

void JNL_UDPConnection::close(int quick)
{
  if (quick || m_state == STATE_RESOLVING)
  {
    m_state=STATE_CLOSED;
    if (m_socket >= 0)
    {
      ::shutdown(m_socket, SHUT_RDWR);
      ::closesocket(m_socket);
    }
    m_socket=-1;
    memset(m_recv_buffer,0,m_recv_buffer_len);
    memset(m_send_buffer,0,m_send_buffer_len);
    m_remote_port=0;
    m_recv_len=m_recv_pos=0;
    m_send_len=m_send_pos=0;
    m_host[0]=0;
    memset(&m_saddr,0,NULL);
  }
  else
  {
    if (m_state == STATE_CONNECTED) m_state=STATE_CLOSING;
  }
}

int JNL_UDPConnection::send_bytes_in_queue(void)
{
  return m_send_len;
}

int JNL_UDPConnection::send_bytes_available(void)
{
  return m_send_buffer_len-m_send_len;
}

int JNL_UDPConnection::send(char *data, int length)
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

int JNL_UDPConnection::send_string(char *line)
{
  return send(line,strlen(line));
}

int JNL_UDPConnection::recv_bytes_available(void)
{
  return m_recv_len;
}

int JNL_UDPConnection::peek_bytes(char *data, int maxlength)
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

int JNL_UDPConnection::recv_bytes(char *data, int maxlength)
{
  
  int ml=peek_bytes(data,maxlength);
  m_recv_len-=ml;
  return ml;
}

int JNL_UDPConnection::getbfromrecv(int pos, int remove)
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

int JNL_UDPConnection::recv_lines_available(void)
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

int JNL_UDPConnection::recv_line(char *line, int maxlength)
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

unsigned long JNL_UDPConnection::get_interface(void)
{
  if (m_socket==-1) return 0;
  struct sockaddr_in sin;
  memset(&sin,0,sizeof(sin));
  socklen_t len=16;
  if (::getsockname(m_socket,(struct sockaddr *)&sin,&len)) return 0;
  return (unsigned long) sin.sin_addr.s_addr;
}
