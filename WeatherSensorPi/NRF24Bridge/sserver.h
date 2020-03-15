#pragma once

#include <sys/socket.h>
#include <cstdint>

#define MAX_CLIENTS 10

class SServer
{
public:
	SServer(int port)
	{
		this->port = (unsigned short int)port;
	}
	~SServer();

	void startServer();
	void pollConnections();
	void closeServer();
	int sendData(int clientId, uint8_t* data, int count);
	int sendDataToAll(uint8_t* data, int count);
	unsigned short port;

private:
	int listener;
	int clients[MAX_CLIENTS] = { 0 };

	void manageConnection(int listener);
	void manageClient(int fd, int clientIndex);
};