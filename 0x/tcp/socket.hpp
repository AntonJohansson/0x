#pragma once

#include <stdint.h>
#include <string>

// Currently designing for AOS
// shouldn't really matter though
// since we don't expect that many
// connections.

struct Socket{
	static constexpr int32_t INVALID{-1};
	int32_t handle{INVALID};
	std::string ip_address{};

	Socket(){}
	Socket(int32_t handle, const std::string& ip_address)
		: handle(handle), ip_address(ip_address) {}
	~Socket();

	void close();
	void set_blocking();
	void set_nonblocking();
	void send(const std::string& data);
	void send_all(const std::string& data);
	// returning string by value will require
	// copy of data!
	std::string recv();

	inline bool valid(){return handle != INVALID;}
};
