#pragma once

#include "network/binary_encoding.hpp"
#include <assert.h>
#include <stdio.h>

enum class PacketType{
	INVALID = 0,
	// output
	SESSION_LIST,
	OBSERVER_MAP,
	PLAYER_MAP,
	// input
	COMMAND,
	TURN_TRANSACTION
};

struct SessionList{
};

struct ObserverMap{
};

struct PlayerMap{
};

struct Command{
	std::vector<std::string_view> words;
};

struct TurnTransaction{
	int resource_amount = 0;
	int q0 = 0, r0 = 0;
	int q1 = 0, r1 = 0;
};

union PacketUnion{
	PacketType type = PacketType::INVALID;
};

inline void encode_packet(BinaryData& data, PacketType type){
	// TODO: a lot of copies going on here
	assert(type != PacketType::INVALID);

	static BinaryData temp;
	size_t size = data.size() + sizeof(uint8_t);

	encode::u32(data, size);
	encode::u8(data, type);

	append(temp, data);
	data = temp;
}

inline PacketUnion decode_packet(BinaryData& data){
	uint32_t initial_size = static_cast<uint32_t>(data.size());

	uint32_t size;
	uint8_t packet_type;
	decode::multiple_integer(data, size);

	switch(static_cast<PacketType>(packet_type)){
			case SESSION_LIST:
			case OBSERVER_MAP:
			case PLAYER_MAP:
			case COMMAND:
			case TURN_TRANSACTION:
			default:
				printf("Unrecognized packet type %i!\n", packet_type);
	}

}

