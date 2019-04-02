#include "server.hpp"
#include "io/poll_set.hpp"
#include "tcp/socket.hpp"
#include "tcp/socket_connection.hpp"
#include "game_refactor.hpp"
#include "hash_map.hpp"
#include "string_helper.hpp"
#include "number_types.hpp"
#include "network/binary_encoding.hpp"
#include "network/serialize_data.hpp"
#include "packet.hpp"

#include "hex_map.hpp"

#include <stdio.h>
#include <thread>
#include <string>
#include <string_view>

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
	static unsigned char buffer[1024];
	auto bytes_received = tcp_socket::recv(s, buffer, 1024);
	std::string_view sv(reinterpret_cast<char*>(buffer), bytes_received);

	if(auto connection_opt = connections.at(s)){
		auto& connection = *connection_opt;

		//std::string_view sv = total_data;
		if(compare_next_word(sv, "create_or_join") || compare_next_word(sv, "coj")){
			// if we're already in a session, leave it
			//if(connection.session_info.active){
			//	game::disconnect_from_session(s, connection.session_info);
			//}

			auto mode 		= get_next_word(sv);
			auto name 		= get_next_word(sv);
			auto players 	= get_next_word(sv);

			game::PlayerMode player_mode = game::NONE;
			if(mode.compare("observer") == 0 || mode.compare("o") == 0){player_mode = game::OBSERVER;}
			else if(mode.compare("player") == 0 || mode.compare("p") == 0){player_mode = game::PLAYER;}

			int max_players;
			if(to_number(players, max_players)){
				std::string name_str{name};
				//connection.session_info = game::create_or_join_session(s, player_mode, name_str, max_players);
				game::create_or_join_lobby(s, player_mode, {
							.name = name_str,
							.max_number_of_players = max_players,
							.map_radius = 3
						});
			}
		}else if(compare_next_word(sv, "list_sessions") || compare_next_word(sv, "ls")){
			BinaryData data;
			// TODO: might not be thread safe
			for(auto& lobby : game::get_lobby_list()){
				encode::string(data, lobby);
			}

			encode_packet(data, PacketType::SESSION_LIST);
			tcp_socket::send_all(s, data.data(), data.size());

			//if(game::active_sessions.size()){
			//	BinaryData data;

			//	game::active_sessions.for_each([&](auto& pair){
			//			auto& session = pair.value;
			//			encode::string(data, session.name);
			//			});

			//	encode_packet(data, PacketType::SESSION_LIST);
			//	printf("session list data size %zu\n", data.size());
			//	tcp_socket::send_all(s, &data[0], data.size());
			//}
		}else{
			// transactions
			uint8_t packet_id = 0;
			BinaryData data(buffer, buffer + bytes_received);
			decode::integer(data, packet_id);

			uint64_t lobby_id;
			decode::integer(data, lobby_id);

			if(packet_id == 3){
				int32_t q0, r0, q1, r1;
				uint32_t res;
				decode::multiple_integers(data, res, q0, r0, q1, r1);
				printf("received transaction (%i,%i) -%i-> (%i,%i)\n", q0,r0,res,q1,r1);

				game::commit_player_turn(lobby_id, res, q0, r0, q1, r1);

				//if(auto game_found = game::active_games.at(connection.session_info.name)){
				//	auto& game = *game_found;

				//	game::complete_turn(game, res, q0, r0, q1, r1);
				//	if(auto session_found = game::active_sessions.at(connection.session_info.name); session_found){
				//		auto& session = *session_found;
				//		game::do_turn(session, game);
				//	}
				//}
			}
		}
	}
}




}
