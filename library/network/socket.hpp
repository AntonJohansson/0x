#pragma once

#include <stdint.h>
#include <string>

// Currently designing for AOS
// shouldn't really matter though
// since we don't expect that many
// connections.

using Socket = int;

namespace tcp_socket{
constexpr Socket INVALID = -1;

extern void set_blocking(Socket s);
extern void set_nonblocking(Socket s);

extern void send(Socket s, 		const unsigned char* data, size_t size);
extern void send_all(Socket s, const unsigned char* data, size_t size);
extern void send(Socket s, 		const std::string& data);
extern void send_all(Socket s, const std::string& data);

extern size_t recv(Socket s, unsigned char* buffer, size_t buffer_size);

extern void close(Socket s);
inline bool valid(Socket s){return s != INVALID;}

}

//struct Socket{
//	static constexpr int32_t INVALID{-1};
//	int32_t handle{INVALID};
//	std::string ip_address{};
//
//	Socket(){}
//	Socket(int32_t handle, const std::string& ip_address)
//		: handle(handle), ip_address(ip_address) {}
//	~Socket();
//
//	void close();
//	void set_blocking();
//	void set_nonblocking();
//	void send(const std::string& data);
//	void send_all(const std::string& data);
//	// returning string by value will require
//	// copy of data!
//	std::string recv();
//};
