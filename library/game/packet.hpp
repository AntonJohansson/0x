#pragma once

#include <serialize/binary_encoding.hpp>
#include <crc/crc32.hpp>
//#include <game_refactor.hpp>
#include <assert.h>
#include <stdio.h>


/*
 *  Packets from the server have the following structure
 *  	+---------------------+---------------+----------+-------------
 *  	| payload_length: u32 | packet_id: u8 | crc: u32 | payload ...
 *  	+---------------------+---------------+----------+-------------
 *
 *  	A "hex" consists of
 *  		+--------+--------+------------+----------------+
 *  		| q: i32 | r: i32 | owner: i32 | resources: u32 |
 *  		+--------+--------+------------+----------------+
 *
 *  	where the payload is
 *  		ERROR_MESSAGE:
 *  			--------------------+-------------+
 *  			 string length: u32 | string data |
 *  			--------------------+-------------+
 *
 *  		OBSERVER_MAP:
 *  			----------+---------------+-----+-----+-----+
 *  			 map info | player scores | hex | ... | hex |
 *  			----------+---------------+-----+-----+-----+
 *
 *  			where "map info" 
 *  				-----------------+-------------------+-------------------+-----
 *  				 map radius: u32 | player count: u32 | current turn: u32 | ...
 *  				-----------------+-------------------+-------------------+-----
 *  			and "player scores" is
 *  				+----------------+----------------+-----+----------------+----------------+
 *  				| client id: u32 | resources: u32 | ... | client_id: u32 | resources: u32 |
 *  				+----------------+----------------+-----+---------------------------------+
 *  				|____________player data__________|                                       |
 *  				|_______________________repeated "player count" times_____________________|
 *
 *  		PLAYER_MAP:
 *  			where the whole map consists of
 *  				-----+-----+-----+-----------
 *  				 hex | hex | ... | hex | ...
 *  				-----+-----+-----+-----------
 *  				| 	 |__6 neighbours___|
 *          |_repeated for every___|
 *            player hex
 *
 * 	Packets to the server have to following structure
 * 		+---------------+---------------+------------
 * 		| packet_id: u8 | lobby_id: u64 | payload...
 * 		+---------------+---------------+------------
 *
 * 		where lobby_id is not included for COMMANDs.
 *
 * 	SESSION_LIST:
 * 	OBSERVER_MAP:
 * 	PLAYER_MAP:
 *
 *
 *
 *
 *
 */

enum class PacketType{
	INVALID = 0,
	SESSION_LIST,
	OBSERVER_MAP,
	PLAYER_MAP,
	COMMAND,
	TURN_TRANSACTION,
	ERROR_MESSAGE,
	CONNECTED_TO_LOBBY,
	ADMIN_COMMAND
};

struct SessionList{
};

struct ObserverMap{
};

struct PlayerMap{
};

struct Command{
	std::string data;
};

struct TurnTransaction{
	uint64_t lobby_id;
	uint32_t resource_amount = 0;
	int32_t q0 = 0, r0 = 0;
	int32_t q1 = 0, r1 = 0;
};

struct AdminPause{
};

struct AdminForceGameState{
};

struct PacketUnion{
	PacketType type = PacketType::INVALID;
	union{
		SessionList session_list;
		ObserverMap observer_map;
		PlayerMap 	player_map;
		Command 		command;
	};
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

	//PacketUnion result = {};
	auto type = static_cast<PacketType>(packet_type);

	switch(type){
			case PacketType::SESSION_LIST:
			case PacketType::OBSERVER_MAP:
			case PacketType::PLAYER_MAP:
			case PacketType::COMMAND:
				//result.command = {std::string(data.begin(), data.begin() + size)};
			case PacketType::TURN_TRANSACTION:
			default:
				printf("Unrecognized packet type %i!\n", packet_type);
	}

}

struct PacketHeader{
	size_t initial_size;
	uint32_t payload_size;
	PacketType packet_type;
};

inline PacketHeader decode_packet_header(BinaryData& data){
	size_t initial_size = data.size();

	uint32_t payload_size;
	uint8_t packet_type;
	decode::multiple_integers(data, payload_size, packet_type);

	return {initial_size, payload_size, static_cast<PacketType>(packet_type)};
}

inline Command decode_command_packet(BinaryData& data, const PacketHeader& header){
	return {std::string(data.begin(), data.end())};
}

inline TurnTransaction decode_transaction_packet(BinaryData& data, const PacketHeader& header){
	uint64_t lobby_id;
	int32_t q0, r0, q1, r1;
	uint32_t resources;
	decode::multiple_integers(data, lobby_id, resources, q0, r0, q1, r1);

	return {lobby_id, resources, q0, r0, q1, r1};
}

inline void encode_error_message(BinaryData& data, const std::string& message){
	encode::string(data, message);
	encode_packet(data, PacketType::ERROR_MESSAGE);
}
