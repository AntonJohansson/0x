#pragma once

#include "socket.hpp"

struct sockaddr;
struct sockaddr_storage;
struct addrinfo;

Socket server_bind(std::string port);
void   server_listen(Socket server, uint32_t backlog);
Socket server_accept(Socket server);
Socket client_bind(std::string ip, std::string port);

// Helpers for dealing with ip
void* get_in_addr(sockaddr* sa);
std::string ntop(sockaddr* sa);
std::string ntop(sockaddr_storage* ss);
std::string ntop(addrinfo* ai);

