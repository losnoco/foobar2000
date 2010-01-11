// Copyright (c) 1999 Lee Patterson
// lee@antws.com
// Modified by Rune Madsen (downey@artemis.dk)

// blocksock.cpp (CBlockingSocketException, CBlockingSocket, CHttpBlockingSocket)
#include <winsock.h>
#include <assert.h>
#include <stdio.h>
#include "blockingsocket.h"

#define ASSERT assert
#define VERIFY ASSERT


// Class CBlockingSocket

void CBlockingSocket::Cleanup()
{
	// doesn't throw an exception because it's called in a catch block
	if(m_hSocket == NULL) return;
	closesocket(m_hSocket);
	m_hSocket = NULL;
}

void CBlockingSocket::Create(int nType /* = SOCK_STREAM */)
{
	ASSERT(m_hSocket == NULL);
	if((m_hSocket = socket(AF_INET, nType, 0)) == INVALID_SOCKET) {
		throw "Create";
	}
}

void CBlockingSocket::Bind(LPCSOCKADDR psa)
{
	ASSERT(m_hSocket != NULL);
	if(bind(m_hSocket, psa, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		throw "Bind";
	}
}

void CBlockingSocket::Listen()
{
	ASSERT(m_hSocket != NULL);
	if(listen(m_hSocket, 5) == SOCKET_ERROR) {
		//throw "Listen";
		throw "Listen";
	}
}

bool CBlockingSocket::Accept(CBlockingSocket& sConnect, LPSOCKADDR psa)
{
	ASSERT(m_hSocket != NULL);
	ASSERT(sConnect.m_hSocket == NULL);
	int nLengthAddr = sizeof(SOCKADDR);
	sConnect.m_hSocket = accept(m_hSocket, psa, &nLengthAddr);
	if(sConnect == INVALID_SOCKET) {
		// no exception if the listen was canceled
		if(WSAGetLastError() != WSAEINTR) {
			throw "Accept";
		}
		return FALSE;
	}
	return TRUE;
}

void CBlockingSocket::Close()
{
	ASSERT(m_hSocket != NULL);
	if(closesocket(m_hSocket) == SOCKET_ERROR) {
		// should be OK to close if closed already
		throw "Close";
	}
	m_hSocket = NULL;
}

void CBlockingSocket::Connect(LPCSOCKADDR psa)
{
	ASSERT(m_hSocket != NULL);
	// should timeout by itself
	if(connect(m_hSocket, psa, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		throw "Connect";
	}
}

int CBlockingSocket::Write(const char* pch, const int nSize, const int nSecs)
{
	int nBytesSent = 0;
	int nBytesThisTime;
	const char* pch1 = pch;
	do {
		nBytesThisTime = Send(pch1, nSize - nBytesSent, nSecs);
		nBytesSent += nBytesThisTime;
		pch1 += nBytesThisTime;
	} while(nBytesSent < nSize);
	return nBytesSent;
}

int CBlockingSocket::Send(const char* pch, const int nSize, const int nSecs)
{
	ASSERT(m_hSocket != NULL);
	// returned value will be less than nSize if client cancels the reading
	FD_SET fd = {1, m_hSocket};
	TIMEVAL tv = {nSecs, 0};
	if(select(0, NULL, &fd, NULL, &tv) == 0) {
		throw "Send timeout";
	}
	int nBytesSent;
	if((nBytesSent = send(m_hSocket, pch, nSize, 0)) == SOCKET_ERROR) {
		throw "Send";
	}
	return nBytesSent;
}

int CBlockingSocket::Receive(char* pch, const int nSize, const int nSecs)
{
	ASSERT(m_hSocket != NULL);
	FD_SET fd = {1, m_hSocket};
	TIMEVAL tv = {nSecs, 0};
	if(select(0, &fd, NULL, NULL, &tv) == 0) {
		throw "Receive timeout";
	}

	int nBytesReceived;
	if((nBytesReceived = recv(m_hSocket, pch, nSize, 0)) == SOCKET_ERROR) {
		throw "Receive";
	}
	return nBytesReceived;
}

int CBlockingSocket::ReceiveDatagram(char* pch, const int nSize, LPSOCKADDR psa, const int nSecs)
{
	ASSERT(m_hSocket != NULL);
	FD_SET fd = {1, m_hSocket};
	TIMEVAL tv = {nSecs, 0};
	if(select(0, &fd, NULL, NULL, &tv) == 0) {
		throw "Receive timeout";
	}

	// input buffer should be big enough for the entire datagram
	int nFromSize = sizeof(SOCKADDR);
	int nBytesReceived = recvfrom(m_hSocket, pch, nSize, 0, psa, &nFromSize);
	if(nBytesReceived == SOCKET_ERROR) {
		throw "ReceiveDatagram";
	}
	return nBytesReceived;
}

int CBlockingSocket::SendDatagram(const char* pch, const int nSize, LPCSOCKADDR psa, const int nSecs)
{
	ASSERT(m_hSocket != NULL);
	FD_SET fd = {1, m_hSocket};
	TIMEVAL tv = {nSecs, 0};
	if(select(0, NULL, &fd, NULL, &tv) == 0) {
		throw "Send timeout";
	}

	int nBytesSent = sendto(m_hSocket, pch, nSize, 0, psa, sizeof(SOCKADDR));
	if(nBytesSent == SOCKET_ERROR) {
		throw "SendDatagram";
	}
	return nBytesSent;
}

void CBlockingSocket::GetPeerAddr(LPSOCKADDR psa)
{
	ASSERT(m_hSocket != NULL);
	// gets the address of the socket at the other end
	int nLengthAddr = sizeof(SOCKADDR);
	if(getpeername(m_hSocket, psa, &nLengthAddr) == SOCKET_ERROR) {
		throw "GetPeerName";
	}
}

void CBlockingSocket::GetSockAddr(LPSOCKADDR psa)
{
	ASSERT(m_hSocket != NULL);
	// gets the address of the socket at this end
	int nLengthAddr = sizeof(SOCKADDR);
	if(getsockname(m_hSocket, psa, &nLengthAddr) == SOCKET_ERROR) {
		throw "GetSockName";
	}
}

//static
CSockAddr CBlockingSocket::GetHostByName(const char* pchName, const USHORT ushPort /* = 0 */)
{
	hostent* pHostEnt = gethostbyname(pchName);
	if(pHostEnt == NULL) {
		throw "GetHostByName";
	}
	ULONG* pulAddr = (ULONG*) pHostEnt->h_addr_list[0];
	SOCKADDR_IN sockTemp;
	sockTemp.sin_family = AF_INET;
	sockTemp.sin_port = htons(ushPort);
	sockTemp.sin_addr.s_addr = *pulAddr; // address is already in network byte order
	return sockTemp;
}

//static
const char* CBlockingSocket::GetHostByAddr(LPCSOCKADDR psa)
{
	hostent* pHostEnt = gethostbyaddr((char*) &((LPSOCKADDR_IN) psa)
				->sin_addr.s_addr, 4, PF_INET);
	if(pHostEnt == NULL) {
		throw "GetHostByAddr";
	}
	return pHostEnt->h_name; // caller shouldn't delete this memory
}

CTelnetSocket::CTelnetSocket()
{
	m_pReadBuf   = new char[nSizeRecv];
	m_nReadBuf   = 0;
	m_wFlags     = 0;
	m_pLoginName = NULL;
}

CTelnetSocket::~CTelnetSocket()
{
	delete [] m_pReadBuf;
	if(m_pLoginName) delete [] m_pLoginName;
}

bool CTelnetSocket::SetLoginName(const char* pname)
{
	m_pLoginName=(char*)malloc(strlen(pname)+1);
	if(!m_pLoginName)
		return false;

	strcpy(m_pLoginName,pname);
	m_pLoginName[strlen(pname)]='\0';
	return true;
}

int CTelnetSocket::ReadLine(char* pch, const int nSize, const int nSecs)
// reads an entire header line through CRLF (or socket close)
// inserts zero string terminator, object maintains a buffer
{
	int nBytesThisTime = m_nReadBuf;
	int nLineLength = 0;
	char* pch1 = m_pReadBuf;
	char* pch2;
	do {
		// look for lf (assume preceded by cr)
		if((pch2 = (char*) memchr(pch1 , '\n', nBytesThisTime)) != NULL) {
			ASSERT((pch2) > m_pReadBuf);
			ASSERT(*(pch2 - 1) == '\r');
			nLineLength = (pch2 - m_pReadBuf) + 1;
			if(nLineLength >= nSize) nLineLength = nSize - 1;
			memcpy(pch, m_pReadBuf, nLineLength); // copy the line to caller
			m_nReadBuf -= nLineLength;
			memmove(m_pReadBuf, pch2 + 1, m_nReadBuf); // shift remaining characters left
			break;
		}
		pch1 += nBytesThisTime;
		nBytesThisTime = Receive(m_pReadBuf + m_nReadBuf, nSizeRecv - m_nReadBuf, nSecs);
		if(nBytesThisTime <= 0) { // sender closed socket or line longer than buffer
			throw "ReadHeaderLine";
		}
		
		//update telnet session
		try 
		{
			Write(m_pReadBuf + m_nReadBuf,nBytesThisTime);
		}
		catch(const char* e) 
		{
			e=e;
			throw "ReadHeaderLine::write";
		}
		
		m_nReadBuf += nBytesThisTime;
	}
	while(TRUE);
	*(pch + nLineLength-2) = '\0';	//add rest of buffer, and remove lfcr
	return nLineLength;
}

int CTelnetSocket::ReadResponse(char* pch, const int nSize, const int nSecs)
// reads remainder of a transmission through buffer full or socket close
// (assume headers have been read already)
{
	int nBytesToRead, nBytesThisTime, nBytesRead = 0;
	if(m_nReadBuf > 0) { // copy anything already in the recv buffer
		memcpy(pch, m_pReadBuf, m_nReadBuf);
		pch += m_nReadBuf;
		nBytesRead = m_nReadBuf;
		m_nReadBuf = 0;
	}
	do { // now pass the rest of the data directly to the caller
		nBytesToRead = min(nSizeRecv, nSize - nBytesRead);
		nBytesThisTime = Receive(pch, nBytesToRead, nSecs);
		if(nBytesThisTime <= 0) break; // sender closed the socket
		pch += nBytesThisTime;
		nBytesRead += nBytesThisTime;
	}
	while(nBytesRead <= nSize);
	return nBytesRead;
}

int CTelnetSocket::Print(const char* msg)
{
	int iBytes;
	char* buf=(char*)malloc(strlen(msg)+3);
	sprintf(buf,"%s\r\n",msg);
	iBytes=Write(buf,strlen(buf));
	free(buf);
	return iBytes;
}
