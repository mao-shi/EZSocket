#include <stdio.h>
#include <time.h>
#include "ezsocket.h"

int main()
{
	EZSocket::Init();

	EZSocket server;
	if (!server.CreateTCPServer(12345))
	{
		printf("CreateTCPServer failed\n");
		return 0;
	}

	char szIpFrom[16] = { 0 };
	EZSocket client;
	while (1)
	{
		client.Shutdown();
		if (!server.WaitForClient(client, szIpFrom))
		{
			printf("WaitForClient failed\n");
			continue;
		}
		printf("WaitForClient %s\n", szIpFrom);

		int flag = 0, len = 0;
		while (1)
		{
			if (!client.RecvHeader(flag, len))
			{
				printf("RecvHeader failed\n");
				break;
			}
			switch (flag)
			{
			case EZSOCKET_HEADER_FLAG_FILE:
				{
					if (!client.RecvFile("D:\\Git\\EZSocket\\file", len))
					{
						printf("RecvFile failed\n");
						break;
					}
					printf("RecvFile\n");
				}
				break;
			case EZSOCKET_HEADER_FLAG_MSG:
				{
					const char *msg = client.RecvMsg(len);
					if (!msg)
					{
						printf("RecvMsg failed\n");
						break;
					}
					printf("RecvMsg %s\n", msg);
				}
				break;
			case EZSOCKET_HEADER_FLAG_HEARTBEAT:
				break;
			default:
				break;
			}
		}
	}

	EZSocket::Clean();
	return 0;
}