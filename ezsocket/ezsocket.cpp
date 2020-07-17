#define _CRT_SECURE_NO_WARNINGS
#include "ezsocket.h"
#include <stdio.h>
#include <malloc.h>
#ifdef WIN32
#else
#include <errno.h>
#endif
#include <time.h>

#ifdef WIN32
#pragma comment(lib, "wsock32")
#endif

#ifndef TCP_NODELAY
#define TCP_NODELAY     0x0001
#endif
#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
//FLAG DATA_LENGTH
//ռ1 15λ
#define EZSOCKET_HEADER_LENGTH 16
#define EZSOCKET_MAX_LENGTH (8*1024)
#undef EZSOCKET_TIMEOUT
#define EZSOCKET_TIMEOUT 16
#define EZSOCKET_HEADER_FLAG_ACK 4

class EZSocketHeader
{
public:
	//TO
	EZSocketHeader(const char *message)
	{
		memcpy(m_szHeader, message, EZSOCKET_HEADER_LENGTH);
	}
	//FROM
	EZSocketHeader(int flag, int len)
	{
		memset(m_szHeader, 0, EZSOCKET_HEADER_LENGTH);
		sprintf(m_szHeader, "%d", flag);
		sprintf(m_szHeader + 1, "%d", len);
	}
	virtual ~EZSocketHeader()
	{}

public:
	//TO
	const char* GetHeader()
	{
		return m_szHeader;
	}
	//FROM
	int GetFlag()
	{
		char szTemp[2] = { 0 };
		memcpy(szTemp, m_szHeader, 1);
		return atoi(szTemp);
	}
	int GetLength()
	{
		char szTemp[16] = { 0 };
		memcpy(szTemp, m_szHeader + 1, 15);
		return atoi(szTemp);
	}

protected:
	char m_szHeader[EZSOCKET_HEADER_LENGTH];
};

EZSocket::EZSocket(SOCKET sock/* = INVALID_SOCKET*/)
{
	m_sock = sock;
	m_buffer = new char[EZSOCKET_MAX_LENGTH];
}
EZSocket::~EZSocket()
{
	Close();
	delete[] m_buffer;
}

int EZSocket::Init()
{
#ifdef WIN32
	WSADATA wsaData;
	WORD version = MAKEWORD(2, 0);
	int ret = WSAStartup(version, &wsaData);
	if (ret)
	{
		return -1;
	}
#endif
	return 0;
}
int EZSocket::Clean()
{
#ifdef WIN32
	return (WSACleanup());
#endif
	return 0;
}

bool EZSocket::Create(int af/* = AF_INET*/, int type/* = SOCK_STREAM*/, int protocol/* = 0*/)
{
	m_sock = socket(af, type, protocol);
	if ( m_sock == INVALID_SOCKET )
	{
		return false;
	}

	int opt = EZSOCKET_MAX_LENGTH;
    int optlen = sizeof(opt); 
	setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (const char*)&opt, optlen);
	setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, optlen);
	opt = 0;
	setsockopt(m_sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, optlen);
	opt = 1000;
	setsockopt(m_sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&opt, optlen);
	setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&opt, optlen);
	return true;
}
bool EZSocket::Bind(unsigned short port)
{
	struct sockaddr_in svraddr = { 0 };
	svraddr.sin_family = AF_INET;
	svraddr.sin_addr.s_addr = INADDR_ANY;
	svraddr.sin_port = htons(port);

	int opt = 1;
	if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0)
	{
		return false;
	}

	int ret = bind(m_sock, (struct sockaddr*)&svraddr, sizeof(svraddr));
	if (ret == SOCKET_ERROR)
	{
		return false;
	}
	return true;
}
bool EZSocket::Listen(int backlog/* = 5*/)
{
	int ret = listen(m_sock, backlog);
	if (ret == SOCKET_ERROR)
	{
		return false;
	}
	return true;
}
int EZSocket::Send(const char* buffer, int len, int flags/* = 0*/)
{
	int bytes = 0;
	int count = 0;
	while (count < len)
	{
		bytes = send(m_sock, buffer + count, len - count, flags);
		if (bytes == -1 || bytes == 0)
		{
			return -1;
		}
		count += bytes;
	}
	return count;
}
int EZSocket::Recv(char* buffer, int len, int flags/* = 0*/)
{
	int bytes = 0;
	int count = 0;
	for (int i = 0; i < EZSOCKET_TIMEOUT; i++)
	{
		bytes = recv(m_sock, buffer + count, len - count, flags);
		if (bytes == -1 || bytes == 0)
		{
			return -1;
		}
		count += bytes;
		if (count >= len)
		{
			break;
		}
	}
	return count;
}
int EZSocket::Close()
{
#ifdef WIN32
	int ret = (closesocket(m_sock));
#else
	int ret = (close(m_sock));
#endif
	m_sock = INVALID_SOCKET;
	return ret;
}
int EZSocket::GetError()
{
#ifdef WIN32
	return (WSAGetLastError());
#else
	return (errno);
#endif
}
int EZSocket::Select(int flag)
{
	int ret = 0;
	struct fd_set fds;
    struct timeval timeout = { 1, 0 };
	FD_ZERO(&fds);
	FD_SET(m_sock, &fds);
	switch (flag)
	{
	case 1:
		ret = select(m_sock + 1, &fds, NULL, NULL, &timeout);
		break;
	case 2:
		ret = select(m_sock + 1, NULL, &fds, NULL, &timeout);
		break;
	case 3:
		ret = select(m_sock + 1, NULL, NULL, &fds, &timeout);
		break;
	default:
		break;
	}
	return ret;
}
bool EZSocket::SendAck()
{
	EZSocketHeader header(EZSOCKET_HEADER_FLAG_ACK, 0);
	int ret = Send(header.GetHeader(), EZSOCKET_HEADER_LENGTH);
	if (ret <= 0)
	{
		return false;
	}
	return true;
}
bool EZSocket::RecvAck()
{
	char buffer[EZSOCKET_HEADER_LENGTH] = { 0 };
	for (int i = 0; i < EZSOCKET_TIMEOUT; i++)
	{
		if (Select(1) > 0)
		{
			int ret = Recv(buffer, EZSOCKET_HEADER_LENGTH);
			if (ret == EZSOCKET_HEADER_LENGTH)
			{
				return true;
			}
			return false;
		}
	}
	return false;
}

EZSocket& EZSocket::operator = (SOCKET sock)
{
	m_sock = sock;
	return (*this);
}
EZSocket::operator SOCKET ()
{
	return m_sock;
}

bool EZSocket::CreateTCPServer(unsigned short port)
{
	if (Create() == false)
	{
		return false;
	}
	if (Bind(port) == false)
	{
		return false;
	}
	if (Listen(port) == false)
	{
		return false;
	}
	return true;
}
bool EZSocket::WaitForClient(EZSocket &sock, char *ip_from/* = NULL*/)
{
	struct sockaddr_in cliaddr = { 0 };
	socklen_t addrlen = sizeof(cliaddr);
	sock = accept(m_sock, (struct sockaddr*)&cliaddr, &addrlen);
	if (sock == SOCKET_ERROR)
	{
		return false;
	}
	if (ip_from != NULL)
	{
		sprintf(ip_from, "%s", inet_ntoa(cliaddr.sin_addr));
	}
	return true;
}

bool EZSocket::CreateTCPClient()
{
	if (Create() == false)
	{
		return false;
	}
	return true;
}
bool EZSocket::ConnectTCPServer(const char* ip, unsigned short port)
{
	struct sockaddr_in svraddr = { 0 };
	svraddr.sin_family = AF_INET;
	svraddr.sin_addr.s_addr = inet_addr(ip);
	svraddr.sin_port = htons(port);
	int ret = connect(m_sock, (struct sockaddr*)&svraddr, sizeof(svraddr));
	if (ret == SOCKET_ERROR)
	{
		return false;
	}
	return true;
}

bool EZSocket::DNSParse(const char* domain, char* ip)
{
	struct hostent *p = NULL;
	if ((p = gethostbyname(domain)) == NULL)
	{
		return false;
	}
	sprintf(ip,
		"%u.%u.%u.%u",
		(unsigned char)p->h_addr_list[0][0],
		(unsigned char)p->h_addr_list[0][1],
		(unsigned char)p->h_addr_list[0][2],
		(unsigned char)p->h_addr_list[0][3]);
	return true;
}
bool EZSocket::RecvHeader(int &flag, int &len)
{
	for (int i = 0; i < EZSOCKET_TIMEOUT; i++)
	{
		if (Select(1) > 0)
		{
			int ret = Recv(m_buffer, EZSOCKET_HEADER_LENGTH);
			if (ret == EZSOCKET_HEADER_LENGTH)
			{
				EZSocketHeader header(m_buffer);
				flag = header.GetFlag();
				len = header.GetLength();
				if (flag == EZSOCKET_HEADER_FLAG_HEARTBEAT)
				{
					if (SendAck() == false)
					{
						return false;
					}
					return true;
				}
				return true;
			}
			return false;
		}
	}
	return false;
}
bool EZSocket::SendFile(const char *filename, pfnFileStreamProgress pfnProgress/* = NULL*/, void *pData/* = NULL*/)
{
	FILE *file = NULL;
	file = fopen(filename, "rb");
	if (!file)
	{
		return false;
	}
	fseek(file, 0L, SEEK_END);
	long len = ftell(file), cur = 0;
	fseek(file, 0L, SEEK_SET);
	EZSocketHeader header(EZSOCKET_HEADER_FLAG_FILE, len);
	int ret = Send(header.GetHeader(), EZSOCKET_HEADER_LENGTH);
	if (ret <= 0)
	{
		fclose(file);
		return false;
	}
	while ((ret = fread(m_buffer, 1, EZSOCKET_MAX_LENGTH, file)) > 0)
	{
		ret = Send(m_buffer, ret);
		if (ret <= 0)
		{
			fclose(file);
			return false;
		}
		if (pfnProgress)
		{
			cur += ret;
			pfnProgress(cur, len, pData);
		}
	}
	fclose(file);
	return RecvAck();
}
bool EZSocket::RecvFile(const char *filename, int len)
{
	FILE *file = NULL;
	file = fopen(filename, "wb+");
	if (!file)
	{
		return false;
	}
	int left = len;
	while (1)
	{
		if (left <= 0)
		{
			break;
		}
		for (int i = 0; i < EZSOCKET_TIMEOUT; i++)
		{
			if (Select(1) > 0)
			{
				int ret = Recv(m_buffer, min(left, EZSOCKET_MAX_LENGTH));
				if (ret > 0)
				{
					left -= ret;
					fwrite(m_buffer, 1, ret, file);
					break;
				}
				fclose(file);
				return false;
			}
		}
	}
	fclose(file);
	return SendAck();
}
bool EZSocket::SendMsg(const char *message)
{
	int len = strlen(message);
	EZSocketHeader header(EZSOCKET_HEADER_FLAG_MSG, len);
	memcpy(m_buffer, header.GetHeader(), EZSOCKET_HEADER_LENGTH);
	memcpy(m_buffer + EZSOCKET_HEADER_LENGTH, message, len);
	int ret = Send(m_buffer, EZSOCKET_HEADER_LENGTH + len);
	if (ret <= 0)
	{
		return false;
	}
	return RecvAck();
}
const char* EZSocket::RecvMsg(int len)
{
	for (int i = 0; i < EZSOCKET_TIMEOUT; i++)
	{
		if (Select(1) > 0)
		{
			int ret = Recv(m_buffer, len);
			if (ret == len)
			{
				m_buffer[len + 1] = 0;
				if (SendAck())
				{
					return m_buffer;
				}
			}
			return NULL;
		}
	}
	return NULL;
}
bool EZSocket::SendHeartBeat()
{
	EZSocketHeader header(EZSOCKET_HEADER_FLAG_HEARTBEAT, 0);
	int ret = Send(header.GetHeader(), EZSOCKET_HEADER_LENGTH);
	if (ret <= 0)
	{
		return false;
	}
	return RecvAck();
}
void EZSocket::Shutdown()
{
	Close();
}