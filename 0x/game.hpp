#pragma once

#include "hash_map.hpp"
#include "hex.hpp"
#include "hex_map.hpp"
#include "tcp/socket.hpp"
#include "pool_allocator.hpp"
#include "packet.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <chrono>

#include "network/serialize_data.hpp"

namespace game{


enum PlayerMode{
	NONE,
	OBSERVER,
	PLAYER
};

static PlayerMode get_player_mode_from_str(const std::string_view& sv){
	if(sv.compare("observer") == 0 || sv.compare("o") == 0){ return OBSERVER; }
	if(sv.compare("player") == 0   || sv.compare("p") == 0){ return PLAYER; }

	return NONE;
}

struct Session{
	std::string name{"default"};
	std::vector<int> observer_handles = {};
	std::vector<int> player_handles = {};
	int observers = 0;
	int players = 0;
	int max_players = 0;
	bool game_in_progress = false;
};

struct SessionInfo{
	bool active = false;
	std::string name{"default"};
	PlayerMode mode = NONE;
};

struct Game{
	bool waiting_on_turn = false;
	int current_turn = 0;
	int current_player_turn = 0;
	std::chrono::high_resolution_clock::time_point begin_turn_time_point;

	int* player_scores = nullptr;
	Session* session = nullptr;
	HexMap* map = nullptr;
};

extern HashMap<std::string, Session> active_sessions;
extern HashMap<std::string, Game> active_games;
extern PoolAllocator<HexMap> map_allocator;

struct CompleteTurnData{
	Game* game;
	int resources;
	int q0, r0, q1, r1;
};
extern std::vector<CompleteTurnData> complete_turn_data;







static void send_observer_data(std::vector<int> observers, Session& session);








static void print_session(Session& session){
	printf("%s:\n\tobservers: %i\n\tplayers %i/%i\n", session.name.c_str(), session.observers, session.players, session.max_players);
}

static void print_game(Game& game){
	// bool waiting_on_turn = false;
	// int current_turn = 0;
	// int current_player_turn = 0;
	// int* player_scores = nullptr;
	// Session* session = nullptr;
	// HexMap* map = nullptr;
	printf("Game: %s\n\tcurrent_turn: %i\n\tcurrent_player_turn: %i\n", game.session->name.c_str(), game.current_turn, game.current_player_turn);
}

static SessionInfo create_or_join_session(int socket_handle, PlayerMode& mode, const std::string& name, int max_players){
	std::cout << active_sessions.size() << std::endl;
	//printf("Entering create_or_join\n");
	if(auto session_found = active_sessions.at(name)){
		auto& session = *session_found;
		if(mode == PLAYER && session.players < session.max_players){
			session.player_handles.push_back(socket_handle);
			session.players++;
		}else if(mode == OBSERVER){
			session.observer_handles.push_back(socket_handle);
			session.observers++;

			if(session.game_in_progress){
				send_observer_data({socket_handle}, session);
			}
		}else{
			// Session full
			return {};
		}

		printf("Joining session\n");
		print_session(session);
		return {true, name, mode};
	}else if(max_players > 0){
		Session session = {};
		session.name = name;
		session.max_players = max_players;

		if(mode == OBSERVER){
			session.observer_handles.push_back(socket_handle);
			session.observers++;
		}else if(mode == PLAYER){
			session.player_handles.push_back(socket_handle);
			session.players++;
		}

		active_sessions.insert({name, session});

				//auto& [key, session] = active_sessions.insert({name, {
		//		name, 
		//		(mode == OBSERVER) ? ({socket_handle}) : ({}),
		//		(mode == PLAYER) ? ({socket_handle}) : ({}),
		//		(mode == OBSERVER) ? 1 : 0,
		//		(mode == PLAYER) ? 1 : 0, 
		//		max_players}
		//		});
		printf("Creating session\n");
		print_session(session);
		return {true, name, mode};
	}else{
		// trying to create session with 0 players, won't fly!
	}

	return {};
}

static void disconnect_from_session(int handle, SessionInfo& info){
	if(auto session_found = active_sessions.at(info.name)){
		auto& session = *session_found;
		switch(info.mode){
			case PLAYER: 		session.players--;		break;
			case OBSERVER: 	session.observers--; 	break;
		};

		{
			auto& v = session.observer_handles;
			v.erase(std::remove(v.begin(), v.end(), handle), v.end());
		}
		printf("Disconnecting from session\n");
		print_session(session);

		if(session.players <= 0 && session.observers <= 0){
			printf("Session empty: closing\n");

			if(auto game_found = active_games.at(info.name)){
				auto& game = *game_found;
				map_allocator.dealloc(game.map);
				active_games.erase(info.name);
			}

			active_sessions.erase(info.name);
		}
	}else{
		printf("Trying to disconnect from session %s that does not exist!\n", info.name.c_str());
	}
}

static void send_observer_data(std::vector<int> observers, Session& session){
	if(auto game_found = active_games.at(session.name); game_found && session.observers > 0){
		auto& game = *game_found;

		BinaryData data;

		serialize_game_state(data, game);
		//printf("sending observer data of size: %lu\n", data.size());

		encode_packet(data, PacketType::OBSERVER_MAP);

		for(auto& s : observers){
			tcp_socket::send_all(s, &data[0], data.size());
		}
	}
}

static void start_game(Session& session){
	auto& [name, game] = active_games.insert({session.name, {}});

	game.player_scores = new int[session.max_players];

	game.map = map_allocator.alloc(5, session.players);
	game.session = &session;

	if(session.observers > 0){
		send_observer_data(session.observer_handles, session);
	}
}

static void complete_turn(Game& game, int amount, int q0, int r0, int q1, int r1){
	auto& cell0 = hex_map::at(*game.map, {q0, r0});
	auto& cell1 = hex_map::at(*game.map, {q1, r1});

	if(amount <= cell0.resources){
		cell0.resources -= amount;
	}else{
		printf("invalid transaction: (%i,%i) has %i, trying to transfer %i\n", q0,r0,cell0.resources,amount);
		game.waiting_on_turn = false;
		return;
	}
	
	if(cell1.player_id != cell0.player_id){
		if(cell1.resources < amount){
			cell1.resources = amount - cell1.resources;
			cell1.player_id = cell0.player_id;
		}
	}else{
		cell1.resources += amount;
	}

	if(cell0.resources == 0){
		cell0.player_id = -1;
	}

	game.waiting_on_turn = false;
}

static void do_turn(Session& session, Game& game){
	//std::cout << "waiting on turn: " << game.waiting_on_turn << "\n";
	//for(int i = 0; i < session.max_players; i++){
	//	std::cout << game.player_scores[i] << "\n";
	//}
	auto dur = std::chrono::high_resolution_clock::now() - game.begin_turn_time_point;

	if(!game.waiting_on_turn){
		game.begin_turn_time_point = std::chrono::high_resolution_clock::now();

		game.current_turn++;
		send_observer_data(session.observer_handles, session);

		BinaryData data;
		// TODO: rebuilding map for every player, might not be necessary
		// only need to track changes
		game.player_scores[game.current_player_turn] = 0;
		hex_map::for_each(*game.map, [&](HexCell& cell){
					if(cell.player_id == game.current_player_turn){
						game.player_scores[game.current_player_turn] += cell.resources;
						encode::multiple_integers(data, cell.q, cell.r, 1, cell.resources);
						//printf("%i, %i\n--------\n", cell.q, cell.r);
						for(auto& n : hex::axial_neighbours({cell.q, cell.r})){
							auto n_cell = hex_map::at(*game.map, n);
							//printf("%i, %i\n", n_cell.q, n_cell.r);
							encode::multiple_integers(data, n_cell.q, n_cell.r, static_cast<int>(n_cell.player_id == cell.player_id), n_cell.resources);
						}
						//printf("--------\n");
					}
				});

		// Check players left in game
		int players_left = 0;
		for(int i = 0; i < session.max_players; i++){
			if(game.player_scores[i] > 0){
				players_left++;
			}
		}

		//if(players_left == 1){
		//	// game has ended, we have a winner
		//	printf("active_games.size(): %i\nmap_allocator.size(): %i\n", active_games.size(), map_allocator.size());
		//	session.game_in_progress = false;
		//	delete[] game.player_scores;
		//	map_allocator.dealloc(game.map);
		//	active_games.erase(session.name);
		//	return;
		//}

		// this should be fine for trolling
		if(game.player_scores[game.current_player_turn] > 0){
			//encode_frame_length(data);
			encode_packet(data, PacketType::PLAYER_MAP);
			//printf("sending player data of size %lu\n", data.size());
			tcp_socket::send_all(session.player_handles[game.current_player_turn], &data[0], data.size());
			game.waiting_on_turn = true;
		}

		// END ROUND
		if(++game.current_player_turn >= session.max_players){
			game.current_player_turn -= session.max_players;
			hex_map::for_each(*game.map, [&](auto& cell){
						if(cell.player_id != -1 && cell.resources < 300){
							cell.resources++;
						}
					});
		}
	}else if(dur > std::chrono::milliseconds(300)){
		//BinaryData data;
		//encode_error_message(data, "time limit crossed!");
		//tcp_socket::send_all(session.player_handles[game.current_player_turn], &data[0], data.size());

		//game.waiting_on_turn = false;
	}
}

static void poll_sessions(){
	active_sessions.for_each([](auto& pair){
			auto& session = pair.value;
			// start game if session is full
			if(!session.game_in_progress && session.players == session.max_players){
				session.game_in_progress = true;
				printf("Session %s full, starting game!\n", session.name.c_str());
				start_game(session);
			}
		});

	for(auto& turn_data : complete_turn_data){
		complete_turn(*turn_data.game, turn_data.resources, turn_data.q0, turn_data.r0, turn_data.q1, turn_data.r1);
	}
	complete_turn_data.clear();

	active_games.for_each([](auto& pair){
			auto& game = pair.value;
			auto& session = *game.session;
			//print_game(game);
			do_turn(session, game);
		});
}

struct PlayerData{
};

}
