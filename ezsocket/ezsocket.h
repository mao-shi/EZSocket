//TCP
#ifndef _EZSOCKET_H_
#define _EZSOCKET_H_

#ifdef WIN32
#include <winsock.h>

typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>

typedef int SOCKET;

//#pragma region define win32 const variable in linux
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

#define EZSOCKET_TIMEOUT 15
#define EZSOCKET_HEADER_FLAG_FILE 1
#define EZSOCKET_HEADER_FLAG_MSG 2
#define EZSOCKET_HEADER_FLAG_HEARTBEAT 3

typedef void (*pfnFileStreamProgress)(long, long, void *pData);//cur total userdata

class EZSocket
{
public:
	EZSocket(SOCKET sock = INVALID_SOCKET);
	virtual ~EZSocket();

public:
	static int Init();
	static int Clean();

protected:
	bool Create(int af = AF_INET, int type = SOCK_STREAM, int protocol = 0);
	bool Bind(unsigned short port);
	bool Listen(int backlog = 5);
	int Send(const char* buffer, int len, int flags = 0);
	int Recv(char* buffer, int len, int flags = 0);
	int Close();
	int GetError();
	int Select(int flag);
	bool SendAck();
	bool RecvAck();

public:
	EZSocket& operator = (SOCKET sock);
	operator SOCKET ();
	//Server
	bool CreateTCPServer(unsigned short port);
	bool WaitForClient(EZSocket &sock, char *ip_from = NULL);
	//Client
	bool CreateTCPClient();
	bool ConnectTCPServer(const char* ip, unsigned short port);
	//Common
	bool DNSParse(const char* domain, char* ip);
	bool RecvHeader(int &flag, int &len);//while call first
	bool SendFile(const char *filename, pfnFileStreamProgress pfnProgress = NULL, void *pData = NULL);
	bool RecvFile(const char *filename, int len);
	bool SendMsg(const char *message);
	const char* RecvMsg(int len);
	bool SendHeartBeat();
	void Shutdown();

protected:
	SOCKET m_sock;
	char *m_buffer;
};

#endif