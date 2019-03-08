#include "server.hpp"
#include "io/poll_set.hpp"
#include "tcp/socket.hpp"
#include "tcp/socket_connection.hpp"
#include "game.hpp"
#include "hash_map.hpp"
#include "string_helper.hpp"
#include "number_types.hpp"

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
		game::SessionInfo session_info = {};
		int handle = -1;
	};

	HashMap<int, Connection> connections;
}

void start(){
	server_socket = server_bind("1111");
	server_socket.set_nonblocking();
	server_listen(server_socket, 10); 
	
	set.add(server_socket.handle, accept_client_connections);

	server_thread = std::thread(poll_fds);
}

void close(){
	should_run = false;
	server_thread.join();
	server_socket.close();
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
	printf("Accepting new connection!\n");

	auto new_socket = server_accept(server_socket);
	new_socket.set_nonblocking();

	printf("New connection %i : %s\n", new_socket.handle, new_socket.ip_address.c_str());

	set.add(new_socket.handle, 						receive_client_data);
	set.on_disconnect(new_socket.handle, 	on_client_disconnect);

	connections.insert({new_socket.handle, Connection{{}, new_socket.handle}});
}

void on_client_disconnect(int s){
	Socket socket(s, "0.0.0.0");

	if(auto connection_found = connections.at(s)){
		auto connection = *connection_found;

		if(connection.session_info.active){
			game::disconnect_from_session(connection.session_info);
		}
	}

	printf("Connection to %i : %s closed!\n", socket.handle, socket.ip_address.c_str());
	socket.close();
}

void receive_client_data(int s){
	Socket socket(s, "0.0.0.0");

	// Receive all data
	std::string total_data;
	std::string incoming_data;
	while(true){
		incoming_data = socket.recv();
		if(incoming_data.empty()) break;

		total_data += incoming_data;
	}

	if(auto connection_opt = connections.at(s)){
		auto& connection = *connection_opt;

		std::string_view sv = total_data;
		if(compare_next_word(sv, "create_or_join") || compare_next_word(sv, "coj")){
			// if we're already in a session, leave it
			if(connection.session_info.active){
				game::disconnect_from_session(connection.session_info);
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

			//std::cout << "|" << mode << "|" << std::endl;
			//std::cout << "|" << name << "|" << std::endl;
			//std::cout << "|" << players << "|" << std::endl;
		}
	}

	//printf("Received data: %s\n", total_data.c_str());
	//socket.send_all(total_data);
}




}
