#include "sserver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>

#define DEBUG 1


void SServer::startServer()
{
	listener = socket(PF_INET, SOCK_STREAM, 0);
	if(listener < 0)
	{
		perror("Error creating socket\n");
		return;
	}

	struct sockaddr_in client_addr;
	client_addr.sin_family = PF_INET;
	client_addr.sin_port = htons(port);
	client_addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(listener, (struct sockaddr*) & client_addr, sizeof(client_addr)) < 0)
	{
		perror("Error in bind()\n");
		return;
	}

#if DEBUG
	printf("Server is listening on the %d port...\n", port);
#endif

	if(listen(listener, 3) < 0)
	{
		perror("Error in listen()\n");
		return;
	}

#if DEBUG
	printf("Waiting for connections...\n");
#endif
}

void SServer::pollConnections()
{
	fd_set readfds;
	int max_fd;
	int active_clients_count;

	FD_ZERO(&readfds);
	FD_SET(listener, &readfds);
	max_fd = listener;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		int fd = clients[i];

		if (fd > 0)
		{
			FD_SET(fd, &readfds);
		}

		max_fd = (fd > max_fd) ? (fd) : (max_fd);
	}

	struct timeval timeout = {0};
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	
	active_clients_count = select(max_fd + 1, &readfds, NULL, NULL, &timeout);

	if (active_clients_count < 0 && (errno != EINTR))
	{
		perror("Error in select():");
		return;
	}

	// If the listener was active than the connection came
	if (FD_ISSET(listener, &readfds))
	{
		manageConnection(listener);
	}

	// Check if there was activity on one of the clients
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		int fd = clients[i];
		if ((fd > 0) && FD_ISSET(fd, &readfds))
		{
			manageClient(fd, i);
		}
	}
}

int SServer::sendData(int clientId, uint8_t* data, int count)
{
	int client = clients[clientId];
	int send_result = send(client, data, count, 0);
	if(send_result < 0)
	{
		perror("Error in send()");
	}
	else
	{
		printf("Data was sent to clientId %d\n", clientId);
	}
	return send_result;
}

int SServer::sendDataToAll(uint8_t* data, int count)
{
	int result = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(clients[i] != 0)
		{
			result |= sendData(i, data, count);
		}
	}

	return result;
}

void SServer::manageConnection(int fd)
{
	struct sockaddr_in client_addr;
	int addrSize = sizeof(client_addr);

	int incom = accept(fd, (struct sockaddr*) & client_addr, (socklen_t*)&addrSize);
	if (incom < 0)
	{
		perror("Error in accept(): ");
		//exit(-1);
		return;
	}
#if DEBUG
	printf("\nNew connection: \nfd = %d \nip = %s:%d\n", incom,
		inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
#endif

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (clients[i] == 0)
		{
			clients[i] = incom;
#if DEBUG
			printf("Managed as client #%d\n", i);
#endif
			break;
		}
	}
}

void SServer::manageClient(int fd, int client_id)
{
	int msg_len = 10;
	char msg[msg_len];
	memset(msg, 0, msg_len);

	struct sockaddr_in client_addr;
	int addrSize = sizeof(client_addr);

	int recvSize = recv(fd, msg, msg_len, 0);
	if (recvSize == 0)
	{
		getpeername(fd, (struct sockaddr*) & client_addr, (socklen_t*)&addrSize);
		printf("Client %d disconnected %s:%d \n", client_id, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		close(fd);
		clients[client_id] = 0;
	}
	else
	{
		msg[recvSize] = '\0';
		printf("Message from %d client: %s\n", client_id, msg);
	}
}
