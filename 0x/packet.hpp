#pragma once

#include "network/binary_encoding.hpp"
#include "crc/crc32.hpp"
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
	TURN_TRANSACTION,
	ERROR_MESSAGE,
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

	BinaryData temp;

	// Encode packet length
	auto size = data.size();
	encode::u32(temp, size);

	// Encode packet id 
	encode::u8(temp, type);

	// Encode payload crc
	auto crc = buffer_crc32(data.data(), data.size());
	encode::u32(temp, crc);
	//printf("encoding CRC: %x\n", crc);

	append(temp, data);
	data = temp;
}

inline PacketUnion decode_packet(BinaryData& data){
	uint32_t initial_size = static_cast<uint32_t>(data.size());

	uint32_t size;
	uint8_t packet_type;
	decode::multiple_integers(data, size);

	switch(static_cast<PacketType>(packet_type)){
			case PacketType::SESSION_LIST:
			case PacketType::OBSERVER_MAP:
			case PacketType::PLAYER_MAP:
			case PacketType::COMMAND:
			case PacketType::TURN_TRANSACTION:
			default:
				printf("Unrecognized packet type %i!\n", packet_type);
	}

}

inline void encode_error_message(BinaryData& data, const std::string& message){
	encode::string(data, message);
	encode_packet(data, PacketType::ERROR_MESSAGE);
}



