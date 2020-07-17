#include <stdio.h>
#include <time.h>
#include "ezsocket.h"

int main()
{
	EZSocket::Init();

	EZSocket client;
	if (!client.CreateTCPClient())
	{
		printf("CreateTCPClient failed\n");
		return 0;
	}

	if (!client.ConnectTCPServer("192.168.0.165", 12345))
	{
		printf("ConnectTCPServer failed\n");
		return 0;
	}

	time_t time_last, time_cur = 0;
	time(&time_last);
	while (1)
	{
		if (!client.SendMsg("Hello World"))
		{
			printf("SendMsg failed\n");
			break;
		}
		printf("SendMsg\n");

		if (!client.SendFile("D:\\Git\\EZSocket\\srcfile"))
		{
			printf("SendFile failed\n");
			break;
		}
		printf("SendFile\n");

		if ((time_cur - time_last) >= EZSOCKET_TIMEOUT)
		{
			if (!client.SendHeartBeat())
			{
				printf("SendHeartBeat failed\n");
				break;
			}
		}

		Sleep(1000);
	}
	client.Shutdown();

	EZSocket::Clean();
	return 0;
}