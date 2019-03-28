#include "server.hpp"
#include "io/poll_set.hpp"
#include "tcp/socket.hpp"
#include "tcp/socket_connection.hpp"
#include "game.hpp"
#include "hash_map.hpp"
#include "string_helper.hpp"
#include "number_types.hpp"
#include "network/binary_encoding.hpp"
#include "network/serialize_data.hpp"

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
		game::SessionInfo session_info = {};
		int handle = -1;
	};

	HashMap<int, Connection> connections;
}

void start(){
	server_socket = server_bind("1111");
	tcp_socket::set_nonblocking(server_socket);
	server_listen(server_socket, 10); 
	
	set.add(server_socket, accept_client_connections);

	//server_thread = std::thread(poll_fds);
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

	connections.insert({new_socket, Connection{addr, {}, new_socket}});
}

void on_client_disconnect(int s){
	if(auto connection_found = connections.at(s)){
		auto connection = *connection_found;

		if(connection.session_info.active){
			game::disconnect_from_session(s, connection.session_info);
		}
			
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
			if(connection.session_info.active){
				game::disconnect_from_session(s, connection.session_info);
			}

			auto mode 		= get_next_word(sv);
			auto name 		= get_next_word(sv);
			auto players 	= get_next_word(sv);

			auto player_mode = game::get_player_mode_from_str(mode);

			int max_players;
			if(to_number(players, max_players)){
				std::string name_str{name};
				connection.session_info = game::create_or_join_session(s, player_mode, name_str, max_players);
			}
		}else if(compare_next_word(sv, "list_sessions") || compare_next_word(sv, "ls")){
			if(game::active_sessions.size()){
				BinaryData data;

				game::active_sessions.for_each([&](auto& pair){
						auto& session = pair.value;
						encode::string(data, session.name);
						});

				encode_packet(data, PacketType::SESSION_LIST);
				printf("session list data size %zu\n", data.size());
				tcp_socket::send_all(s, &data[0], data.size());
			}
		}else{
			uint8_t packet_id = 0;
			BinaryData data(buffer, buffer + bytes_received);
			decode::integer(data, packet_id);

			if(packet_id == 3){
				int res, q0, r0, q1, r1;
				decode::multiple_integers(data, res, q0, r0, q1, r1);

				if(auto game_found = game::active_games.at(connection.session_info.name)){
					auto& game = *game_found;

					game::complete_turn(game, res, q0, r0, q1, r1);
					if(auto session_found = game::active_sessions.at(connection.session_info.name); session_found){
						auto& session = *session_found;
						game::do_turn(session, game);
					}
				}
			}
		}
	}
}




}
