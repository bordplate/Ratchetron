#ifndef SOCKET_H
#define SOCKET_H

#include <sys/socket.h>

#define getPort(p1x, p2x) ((p1x * 256) + p2x)

#define FAILED -1
#define NONE -1

static int slisten(int port, int backlog)
{
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if(s < 0)
	{
		return FAILED;
	}

	struct sockaddr_in sa;
	socklen_t sin_len = sizeof(sa);
	memset(&sa, 0, sin_len);

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	bind(s, (struct sockaddr *)&sa, sin_len);
	listen(s, backlog);

	return s;
}

static void sclose(int *socket_e)
{
	if(*socket_e != NONE)
	{
		shutdown(*socket_e, SHUT_RDWR);
		socketclose(*socket_e);
		*socket_e = NONE;
	}
}

#endif