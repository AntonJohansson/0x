#include "socket.hpp"

#include <stdio.h>
//#include <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <errno.h>
//#include <string.h>
//#include <sys/types.h>
#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netdb.h>
//#include <arpa/inet.h>
//#include <sys/wait.h>
#include <fcntl.h>

namespace tcp_socket{

void set_blocking(Socket s){
	// Clear O_NONBLOCK flag
	int opts = fcntl(s, F_GETFL);
	opts = opts & (~O_NONBLOCK);
	fcntl(s, F_SETFL, opts);
}

void set_nonblocking(Socket s){
	// Set O_NONBLOCK flag
	fcntl(s, F_SETFL, O_NONBLOCK);
}

void send(Socket s, const unsigned char* data, size_t size){
	if(::send(s, data, size, 0) == 1){
		if(errno == EAGAIN || errno == EWOULDBLOCK){
			//printf("EAGAIN/EWOULDBLOCK for %i\n", handle);
			return;
		}else if(errno == ECONNRESET){
			// Connection closed by client
			printf("Connection %i closed by client\n", s);
			return;
		}else{
			printf("Send failed %s, %i\n", std::strerror(errno), errno);
			return;
		}
	}
}

void send_all(Socket s, const unsigned char* data, size_t size){
	size_t total  = 0;
	size_t nbytes = 0;

	while(total < size){
		nbytes = ::send(s, data + total, size - total, 0);

		if(nbytes == -1){
			if(errno == EAGAIN || errno == EWOULDBLOCK){
				//printf("EAGAIN/EWOULDBLOCK for %i\n", handle);
				return;
			}else if(errno == ECONNRESET){
				// Connection closed by client
				printf("Connection %i closed by client\n", s);
				return;
			}else{
				printf("Send failed %s, %i\n", std::strerror(errno), errno);
				return;
			}
		}

		total += nbytes;
	}
}

void send(Socket s, const std::string& data){
	send(s, reinterpret_cast<const unsigned char*>(data.data()), data.size());
}

void send_all(Socket s, const std::string& data){
	send_all(s, reinterpret_cast<const unsigned char*>(data.data()), data.size());
}

static constexpr int32_t MAXDATASIZE = 128;
size_t recv(Socket s, unsigned char* buffer, size_t buffer_size){
	//static char buffer[MAXDATASIZE];
	static int32_t nbytes = 0;

	if(nbytes = ::recv(s, buffer, buffer_size-1, 0); nbytes <= 0){
		if(errno == ECONNRESET || nbytes == 0){
			// Connection closed
			printf("Recv error for %i\n", s);
			return 0;
		}else if(errno == EAGAIN || errno == EWOULDBLOCK){
			//printf("EAGAIN/EWOULDBLOCK for %i\n", handle);
			return 0;
		}else{
			printf("Send failed %s, %i\n", std::strerror(errno), errno);
			return 0;
		}
	}

	return nbytes;
}

void close(Socket s){
	::close(s);
}

void shutdown(Socket s){
	::shutdown(s, SHUT_RDWR);
}


}

//void Socket::send(const std::string& data){
//	
//}
//
//void Socket::send_all(const std::string& data){
//	uint64_t total  = 0;
//	uint64_t nbytes = 0;
//
//	while(total < data.size()){
//		nbytes = ::send(handle, data.data() + total, data.size() - total, 0);
//
//		if(nbytes == -1){
//			if(errno == EAGAIN || errno == EWOULDBLOCK){
//				//printf("EAGAIN/EWOULDBLOCK for %i\n", handle);
//				return;
//			}else if(errno == ECONNRESET){
//				// Connection closed by client
//				printf("Connection %i closed by client\n", handle);
//				return;
//			}else{
//				printf("Send failed %s, %i\n", std::strerror(errno), errno);
//				return;
//			}
//		}
//
//		total += nbytes;
//	}
//}

// returning string by value will require
// copy of data!
//constexpr int32_t MAXDATASIZE = 128;
//std::string Socket::recv(){
//	static char buffer[MAXDATASIZE];
//constexpr int32_t MAXDATASIZE = 128;
//	static int32_t nbytes = 0;
//
//	if(nbytes = ::recv(handle, buffer, MAXDATASIZE-1, 0); nbytes <= 0){
//		if(errno == ECONNRESET || nbytes == 0){
//			// Connection closed
//			printf("Recv error for %i\n", handle);
//			return {};
//		}else if(errno == EAGAIN || errno == EWOULDBLOCK){
//			//printf("EAGAIN/EWOULDBLOCK for %i\n", handle);
//			return {};
//		}else{
//			printf("Send failed %s, %i\n", std::strerror(errno), errno);
//			return {};
//		}
//	}
//
//	return std::string(buffer, nbytes);
//}


