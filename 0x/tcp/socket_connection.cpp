#include "socket_connection.hpp"

#include <stdio.h>
#include <cstring>
//#include <stdlib.h>
#include <unistd.h>
#include <cerrno>
//#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
//#include <netinet/in.h>
#include <netdb.h>

// inet_ntop
#include <arpa/inet.h>
//#include <sys/wait.h>
//#include <fcntl.h>

Socket server_bind(std::string port){
	addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags		= AI_PASSIVE;

	addrinfo* server_info;
	if(int err = getaddrinfo(nullptr, port.c_str(), &hints, &server_info); err != 0){
		printf("Error in getaddrinfo(): %s\n", gai_strerror(err));
	}

	int sock = -1;
	for(auto p = server_info; p != nullptr; p = p->ai_next){
		sock = socket(p->ai_family,
									p->ai_socktype,
									p->ai_protocol);

		if(sock == -1){
			printf("Error in socket(): %s\n", std::strerror(errno));
			continue;
		}

		static int yes = 1;
		if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
			printf("Error in setsockopt(): %s\n", std::strerror(errno));
			return {};
		}

		if(bind(sock, p->ai_addr, p->ai_addrlen) == -1){
			::close(sock);
			printf("Error in bind(): %s\n", std::strerror(errno));
			continue;
		}

		// if we get here: sock is set up and bound
		break;
	}

	auto bound_ip = ntop(server_info);
	freeaddrinfo(server_info);

	if(sock == -1){
		printf("Failed to bind socket!\n");
		return {};
	}

	return Socket(sock, bound_ip);
}

void server_listen(Socket server, uint32_t backlog){
	if(listen(server.handle, backlog) == -1){
		printf("Listen failed\n");
	}
}

Socket server_accept(Socket server){
	static sockaddr_storage their_addr;
	static socklen_t sin_size;

	sin_size = sizeof(their_addr);
	int sock = accept(server.handle, (sockaddr*)&their_addr, &sin_size);

	if(sock == -1){
		printf("accept() failed!\n");
		return {};
	}

	return Socket(sock, ntop(&their_addr));
}

Socket client_bind(std::string ip, std::string port){
	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	addrinfo* server_info;
	if(int err = getaddrinfo(ip.c_str(), port.c_str(), &hints, &server_info); err != 0){
		printf("Error: client_bind()::getaddrinfo: %s\n", gai_strerror(err));
		return {};
	}

	int sock = -1;
	addrinfo* p;
	for(p = server_info; p != nullptr; p = p->ai_next){
		sock = socket(p->ai_family,
									p->ai_socktype,
									p->ai_protocol);

		if(sock == -1){
			printf("Error: socket() failed: %s\n", std::strerror(errno));
			continue;
		}

		if(int err = connect(sock, p->ai_addr, p->ai_addrlen); err == -1){
			::close(sock);
			printf("Error: connect() failed: %s\n", std::strerror(errno));
			continue;
		}

		// if we get here the sock is set up and bound
		break;
	}

	auto server_ip = ntop(server_info);
	freeaddrinfo(server_info);

	if(p == nullptr){
		printf("Error: failed to connect\n");
	}else if(sock == -1){
		printf("Error: failed to bind socket\n");
	}

	return Socket(sock, server_ip);
}





// Helpers for dealing with ip
void* get_in_addr(sockaddr* sa){
	if(sa->sa_family == AF_INET){
		return &(((sockaddr_in*)sa)->sin_addr);
	}else{
		return &(((sockaddr_in6*)sa)->sin6_addr);
	}
}

std::string ntop(sockaddr* sa){
	static char s[INET6_ADDRSTRLEN];
	auto in_addr = get_in_addr(sa);
	inet_ntop(sa->sa_family, in_addr, s, sizeof(s));

	return std::string(s);
}

std::string ntop(sockaddr_storage* ss){
	return ntop((sockaddr*)ss);
}

std::string ntop(addrinfo* ai){
	return ntop((sockaddr*)ai->ai_addr);
}
