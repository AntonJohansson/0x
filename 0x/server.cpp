#include <io/poll_set.hpp>
#include <network/socket.hpp>
#include <network/socket_connection.hpp>
#include <memory/hash_map.hpp>
#include <other/string_helper.hpp>
#include <other/number_types.hpp>
#include <serialize/binary_encoding.hpp>
#include <serialize/serialize_data.hpp>
#include <game/packet.hpp>
#include <game/hex_map.hpp>

#include "server.hpp"
#include "game_refactor.hpp"

#include <stdio.h>
#include <thread>
#include <string>
#include <string_view>

#include <iostream>

namespace server{
namespace{
	PollSet set;
	Socket server_socket;
	bool should_run = true;

	std::thread server_thread;

	struct Connection{
		std::string ip_address = "";
		//game::SessionInfo session_info = {};
		game::LobbyId lobby_id = 0;
		int handle = -1;
	};

	HashMap<int, Connection> connections;
}

static void observer_data_callback(game::ClientId client_id, 
		uint32_t map_radius, 
		uint32_t player_count,
		uint32_t current_turn,
		std::vector<game::PlayerScores> player_scores,
		std::vector<const HexCell*> map){
	BinaryData data;
	encode::multiple_integers(data, map_radius, player_count, current_turn);

	for(auto& [id, score] : player_scores){
		encode::multiple_integers(data, id, score);
	}

	for(auto& hex_ptr : map){
		encode::multiple_integers(data, hex_ptr->q, hex_ptr->r, hex_ptr->player_id, hex_ptr->resources);
	}

	encode_packet(data, PacketType::OBSERVER_MAP);

	tcp_socket::send_all(client_id, data.data(), data.size());
}

static void player_data_callback(game::ClientId client_id, std::vector<game::HexPlayerData> player_map){
	//printf("sending player data to %i\n", client_id);
	BinaryData data;
	for(auto& [hex, neighbours] : player_map){
		encode::multiple_integers(data, hex->q, hex->r, static_cast<uint32_t>(1), hex->resources);
		for(auto& n : neighbours){
			int id = 0;
			if(n->player_id == hex->player_id){id = 1;}
			else if(n->player_id != -1){id = 2;}
			encode::multiple_integers(data, n->q, n->r, static_cast<uint32_t>(id), n->resources);
		}
	}

	//printf("size: %zu\n", player_map.size());
	//printf("sending player map of size %zu\n", data.size());

	encode_packet(data, PacketType::PLAYER_MAP);

	tcp_socket::send_all(client_id, data.data(), data.size());
}

static void error_message_callback(game::ClientId client_id, const std::string& message){
	BinaryData data;
	encode::string(data, message);
	encode_packet(data, PacketType::ERROR_MESSAGE);
	tcp_socket::send_all(client_id, data.data(), data.size());
}

static void connected_to_lobby_callback(game::ClientId client_id, game::LobbyId lobby_id){
	if(auto connection_found = connections.at(client_id); connection_found){
		auto& connection = *connection_found;
		connection.lobby_id = lobby_id;
	}


	BinaryData data;
	encode::integer(data, lobby_id);
	encode_packet(data, PacketType::CONNECTED_TO_LOBBY);
	tcp_socket::send_all(client_id, data.data(), data.size());
}

void start(){
	server_socket = server_bind("1111");
	tcp_socket::set_nonblocking(server_socket);
	server_listen(server_socket, 10); 
	
	set.add(server_socket, accept_client_connections);

	game::set_observer_data_callback(observer_data_callback);
	game::set_player_data_callback(player_data_callback);
	game::set_error_callback(error_message_callback);
	game::set_connected_to_lobby_callback(connected_to_lobby_callback);

	server_thread = std::thread(poll_fds);
}

void close(){
	should_run = false;
	server_thread.join();
	tcp_socket::close(server_socket);
}

bool is_running(){
	return should_run;
}

void poll_fds(){
	while(should_run){
		set.poll();
	}
}























// Callback functions for fds
//

void accept_client_connections(int s){
	auto [new_socket, addr] = server_accept(server_socket);
	tcp_socket::set_nonblocking(new_socket);

	printf("New connection from %s -> %i\n", addr.c_str(), new_socket);

	set.add(new_socket, 						receive_client_data);
	set.on_disconnect(new_socket, 	on_client_disconnect);

	connections.insert({new_socket, Connection{addr, new_socket}});
}

void on_client_disconnect(int s){
	if(auto connection_found = connections.at(s)){
		auto connection = *connection_found;

		game::disconnect_from_lobby(s, connection.lobby_id);

		connections.erase(s);

		//if(connection.session_info.active){
		//	game::disconnect_from_session(s, connection.session_info);
		//}
			
		printf("Connection to %s -> %i :  closed!\n", connection.ip_address.c_str(), s);
	}

	tcp_socket::close(s);
}

static void handle_server_commands(){
}

void receive_client_data(int s){
	static uint8_t buffer[1024];
	size_t bytes_received = tcp_socket::recv(s, buffer, 1024);
	BinaryData data(buffer, buffer + bytes_received);


	
	printf("1\n");
	if(auto connection_opt = connections.at(s)){
		printf("2\n");
		auto& connection = *connection_opt;
		auto header = decode_packet_header(data);
		switch(header.packet_type){
			case PacketType::INVALID:
				printf("3\n");
				error_message_callback(s, "Invalid packet type.");
				break;
			case PacketType::OBSERVER_MAP:
				printf("4\n");
				//auto command_packet = decode_observer_map(data);
				break;
			case PacketType::COMMAND:{
				printf("5\n");
				auto [str] = decode_command_packet(data, header);
				std::string_view sv(str);

				if(compare_next_word(sv, "create_or_join") || compare_next_word(sv, "coj")){
					auto player_mode_sv			= get_next_word(sv);
					auto lobby_name_sv			= get_next_word(sv);
					auto players_sv 				= get_next_word(sv);
					auto radius_sv 					= get_next_word(sv);
					auto restart_on_win_sv 	= get_next_word(sv);

					game::Settings settings;

					game::PlayerMode player_mode = game::NONE;
					std::string lobby_name;
					uint32_t players;
					uint32_t radius;
					bool restart_on_win;

					if(player_mode_sv.size() && lobby_name_sv.size()){
						if(player_mode_sv.compare("observer") == 0 || player_mode_sv.compare("o") == 0){player_mode = game::OBSERVER;}
						else if(player_mode_sv.compare("player") == 0 || player_mode_sv.compare("p") == 0){player_mode = game::PLAYER;}
						else{
							player_mode = game::OBSERVER;
						}

						settings.name = std::string(lobby_name_sv);
					}

					if(players_sv.size() && to_number(players_sv, players)){
						settings.max_number_of_players = players;
					}else{
						settings.max_number_of_players = 6;
					}

					if(radius_sv.size() && to_number(radius_sv, radius)){
						settings.map_radius = radius;
					}else{
						settings.map_radius = 3;
					}

					if(restart_on_win_sv.size()){
						if(restart_on_win_sv.compare("true") == 0){
							settings.restart_on_win = true;
						}else{
							settings.restart_on_win = false;
						}
					}

					game::create_or_join_lobby(s, player_mode, settings);
				}else if(compare_next_word(sv, "list_sessions") || compare_next_word(sv, "ls")){
					BinaryData data;
					// TODO: might not be thread safe
					for(auto& lobby : game::get_lobby_list()){
						encode::string(data, lobby);
					}

					encode_packet(data, PacketType::SESSION_LIST);
					tcp_socket::send_all(s, data.data(), data.size());
				}
				break;

			}
			case PacketType::TURN_TRANSACTION:{
				auto [lobby_id,res,q0,r0,q1,r1] = decode_transaction_packet(data, header);
				std::cout 
					<< lobby_id << " " 
					<< res << " "
					<< q0 << " "
					<< r0 << " "
					<< q1 << " "
					<< r1 << "\n";
				game::commit_player_turn(lobby_id, res, q0, r0, q1, r1);
			}
				break;
			case PacketType::ADMIN_COMMAND:
				//auto transaction_packet = decode_command_packet(data);
				break;
			default:
				error_message_callback(s, "Unknown packet type " + std::to_string(static_cast<int>(header.packet_type)));
				break;
		}
	}
}




}
