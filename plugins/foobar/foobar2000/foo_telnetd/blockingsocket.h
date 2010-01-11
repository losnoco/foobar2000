// Copyright (c) 1999 Lee Patterson
// lee@antws.com
// Modified by Rune Madsen (downey@artemis.dk)

// blocksock.h

// needs winsock.h in the precompiled headers

typedef const struct sockaddr* LPCSOCKADDR;

class CSockAddr : public sockaddr_in {
public:
	// constructors
	CSockAddr()
		{ sin_family = AF_INET;
		  sin_port = 0;
		  sin_addr.s_addr = 0; } // Default
	CSockAddr(const SOCKADDR& sa) { memcpy(this, &sa, sizeof(SOCKADDR)); }
	CSockAddr(const SOCKADDR_IN& sin) { memcpy(this, &sin, sizeof(SOCKADDR_IN)); }
	CSockAddr(const ULONG ulAddr, const USHORT ushPort = 0) // parms are host byte ordered
		{ sin_family = AF_INET;
		  sin_port = htons(ushPort);
	      sin_addr.s_addr = htonl(ulAddr); }
	CSockAddr(const char* pchIP, const USHORT ushPort = 0) // dotted IP addr string
		{ sin_family = AF_INET;
		  sin_port = htons(ushPort);
		  sin_addr.s_addr = inet_addr(pchIP); } // already network byte ordered
	// Return the address in dotted-decimal format
	char* DottedDecimal()
		{ return inet_ntoa(sin_addr); } // constructs a new CString object
	// Get port and address (even though they're public)
	USHORT Port() const
		{ return ntohs(sin_port); }
	ULONG IPAddr() const
		{ return ntohl(sin_addr.s_addr); }
	// operators added for efficiency
	const CSockAddr& operator=(const SOCKADDR& sa)
		{ memcpy(this, &sa, sizeof(SOCKADDR));
		  return *this; }
	const CSockAddr& operator=(const SOCKADDR_IN& sin)
		{ memcpy(this, &sin, sizeof(SOCKADDR_IN));
		  return *this; }
	operator SOCKADDR()
		{ return *((LPSOCKADDR) this); }
	operator LPSOCKADDR()
		{ return (LPSOCKADDR) this; }
	operator LPSOCKADDR_IN()
		{ return (LPSOCKADDR_IN) this; }
};

// member functions truly block and must not be used in UI threads
// use this class as an alternative to the MFC CSocket class
class CBlockingSocket 
{
public:
	enum {DEFAULT_TIMEOUT=10};
	SOCKET m_hSocket;
	CBlockingSocket() { m_hSocket = NULL; }
	void Cleanup();
	void Create(int nType = SOCK_STREAM);
	void Close();
	void Bind(LPCSOCKADDR psa);
	void Listen();
	void Connect(LPCSOCKADDR psa);
	bool Accept(CBlockingSocket& s, LPSOCKADDR psa);
	int Send(const char* pch, const int nSize, const int nSecs=DEFAULT_TIMEOUT);
	int Write(const char* pch, const int nSize, const int nSecs=DEFAULT_TIMEOUT);
	int Receive(char* pch, const int nSize, const int nSecs=DEFAULT_TIMEOUT);
	int SendDatagram(const char* pch, const int nSize, LPCSOCKADDR psa, 
		const int nSecs=10);
	int ReceiveDatagram(char* pch, const int nSize, LPSOCKADDR psa, 
		const int nSecs=10);
	void GetPeerAddr(LPSOCKADDR psa);
	void GetSockAddr(LPSOCKADDR psa);
	static CSockAddr GetHostByName(const char* pchName, 
		const USHORT ushPort = 0);
	static const char* GetHostByAddr(LPCSOCKADDR psa);
	operator SOCKET()
		{ return m_hSocket; }
};

class CTelnetSocket : public CBlockingSocket
{
public:
	enum {nSizeRecv = 1024}; // max receive buffer size (> hdr line length)
	enum {FLAG_VALIDATED=1,FLAG_PLAYING=2};
	CTelnetSocket();
	~CTelnetSocket();
	int ReadLine(char* pch, const int nSize, const int nSecs=DEFAULT_TIMEOUT);
	int ReadResponse(char* pch, const int nSize, const int nSecs=DEFAULT_TIMEOUT);
	int Print(const char* msg);
	bool SetLoginName(const char* name);

public:
	//login information
	char* m_pLoginName;
	WORD m_wFlags;

private:
	char* m_pReadBuf; // read buffer
	int m_nReadBuf; // number of bytes in the read buffer

};
