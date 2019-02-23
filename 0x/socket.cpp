#include "socket.hpp"

#include <stdio.h>
//#include <stdlib.h>
#include <cstring>
#include <unistd.h>
//#include <errno.h>
//#include <string.h>
//#include <sys/types.h>
#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netdb.h>
//#include <arpa/inet.h>
//#include <sys/wait.h>
#include <fcntl.h>

Socket::~Socket(){
}

void Socket::close(){
	printf("Closing socket %i\n", handle);
	::close(handle);
	handle = INVALID;
}

void Socket::set_blocking(){
	// Clear O_NONBLOCK flag
	int opts = fcntl(handle, F_GETFL);
	opts = opts & (~O_NONBLOCK);
	fcntl(handle, F_SETFL, opts);
}

void Socket::set_nonblocking(){
	// Set O_NONBLOCK flag
	fcntl(handle, F_SETFL, O_NONBLOCK);
}

void Socket::send(const std::string& data){
	if(::send(handle, data.c_str(), data.size(), 0) == 1){
		if(errno == EAGAIN || errno == EWOULDBLOCK){
			printf("EAGAIN/EWOULDBLOCK for %i\n", handle);
			return;
		}else if(errno == ECONNRESET){
			// Connection closed by client
			printf("Connection %i closed by client\n", handle);
			return;
		}else{
			printf("Send failed %s, %i\n", std::strerror(errno), errno);
			return;
		}
	}
}

void Socket::send_all(const std::string& data){
	uint64_t total  = 0;
	uint64_t nbytes = 0;

	while(total < data.size()){
		nbytes = ::send(handle, data.data() + total, data.size() - total, 0);

		if(nbytes == -1){
			if(errno == EAGAIN || errno == EWOULDBLOCK){
				printf("EAGAIN/EWOULDBLOCK for %i\n", handle);
				return;
			}else if(errno == ECONNRESET){
				// Connection closed by client
				printf("Connection %i closed by client\n", handle);
				return;
			}else{
				printf("Send failed %s, %i\n", std::strerror(errno), errno);
				return;
			}
		}

		total += nbytes;
	}
}

// returning string by value will require
// copy of data!
constexpr int32_t MAXDATASIZE = 128;
std::string Socket::recv(){
	static char buffer[MAXDATASIZE];
	static int32_t nbytes = 0;

	if(nbytes = ::recv(handle, buffer, MAXDATASIZE-1, 0); nbytes <= 0){
		if(errno == ECONNRESET || nbytes == 0){
			// Connection closed
			printf("Recv error for %i\n", handle);
			return {};
		}else if(errno == EAGAIN || errno == EWOULDBLOCK){
			printf("EAGAIN/EWOULDBLOCK for %i\n", handle);
			return {};
		}else{
			printf("Send failed %s, %i\n", std::strerror(errno), errno);
			return {};
		}
	}

	return std::string(buffer, nbytes);
}


