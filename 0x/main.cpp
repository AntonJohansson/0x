#include "socket.hpp"
#include "server_connection.hpp"
#include "poll_set.hpp"
#include "hash_map.hpp"
#include "game.hpp"

#include <vector>
#include <string>
#include <iostream>
#include <string_view>
#include <string>
#include <charconv>

#include <thread>

bool should_run = true;

struct Connection{
	game::SessionInfo session_info = {};
	int handle = -1;
};
HashMap<int, Connection> connections;


#include "binary_encoding.hpp"
#include <iostream>
void test_binary_decoding(){
	uint8_t  u8  = 255u;
	uint16_t u16 = 65535u;
	uint32_t u32 = 4294967295u;
	uint64_t u64 = 18446744073709551615u;

	std::cout 
		<< "Testing binary encoding/decoding:\n"
		<< "\t" << (int)u8 << "\n"
		<< "\t" << u16 << "\n"
		<< "\t" << u32 << "\n"
		<< "\t" << u64 << "\n";

	BinaryData data;
	encode::uint(data, u8);  // 1 byte
	encode::uint(data, u16); // 2 bytes
	encode::uint(data, u32); // 4 bytes
	encode::uint(data, u64); // 8 bytes

	std::cout << "\n\tEncoded data:\n\t\t" << data.data() << "\n\t\tsize: " << data.size() << "\n";

	std::cout << "\n\tDecoded data:"
		<< "\n\t\t" << decode::uint<uint8_t>(data)
		<< "\n\t\t" << decode::uint<uint16_t>(data)
		<< "\n\t\t" << decode::uint<uint32_t>(data)
		<< "\n\t\t" << decode::uint<uint64_t>(data) << "\n";
}




std::string_view get_next_word(std::string_view& sv){
	while(sv[0] == ' ' || sv[0] == '\n')sv.remove_prefix(1);

	auto i = sv.find_first_of(" \n");
	auto ret = sv.substr(0,i);
	sv.remove_prefix(i);

	return ret;
}

bool compare_next_word(std::string_view& sv, const std::string& word){
	while(sv[0] == ' ' || sv[0] == '\n')sv.remove_prefix(1);

	auto i = sv.find_first_of(" \n");
	if(sv.substr(0,i).compare(word) == 0){
		sv.remove_prefix(i);
		return true;
	}

	return false;
}

// Requires data(), size() member functions
// could probably add some compile time checks
template<typename DataT, typename NumberT>
bool to_number(DataT& str, NumberT& number){
	if(auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), number); ec == std::errc()){
		return true;
	}

	return false;
}









void recieve_data(int s){
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
				connection.session_info = game::create_or_join_session(player_mode, name_str, max_players);
			}

			//std::cout << "|" << mode << "|" << std::endl;
			//std::cout << "|" << name << "|" << std::endl;
			//std::cout << "|" << players << "|" << std::endl;
		}
	}

	//printf("Received data: %s\n", total_data.c_str());
	//socket.send_all(total_data);
}

void on_disconnect(int s){
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





void input_loop(PollSet& set){
	while(should_run){
		set.poll();
	}
}




int main(){
	test_binary_decoding();
	return 0;

	PollSet set;

	auto server = server_bind("1111");
	server.set_nonblocking();
	
	server_listen(server, 10);

	set.add(server.handle, [&](int){
				printf("Accepting new connection!\n");

				auto new_socket = server_accept(server);
				new_socket.set_nonblocking();

				printf("New connection %i : %s\n", new_socket.handle, new_socket.ip_address.c_str());

				set.add(new_socket.handle, recieve_data);
				set.on_disconnect(new_socket.handle, on_disconnect);

				connections.insert({new_socket.handle, Connection{{}, new_socket.handle}});
			});

	set.add(0, [](int){
				std::string line;
				std::getline(std::cin, line);
				if(line == "quit"){
					should_run = false;
				}
			});

	std::thread input_thread(input_loop, std::ref(set));

	while(should_run){
		game::poll_sessions();

		// at start of new game send inital board to observers

		// each round:
		// 	receive transactions
		// 	ready to start?
		// 	filter transactions
		// 	send transactions to observers so that they can be animated
		// 	apply transactions to map
	}

	input_thread.join();

	server.close();
}
