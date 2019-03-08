#pragma once

#include "hash_map.hpp"
#include "hex.hpp"
#include "tcp/socket.hpp"

#include <string>
#include <string_view>
#include <vector>

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
	HexagonalMap* map = nullptr;
};

extern HashMap<std::string, Session> active_sessions;
extern HashMap<std::string, Game> active_games;








static void send_observer_data(std::vector<int> observers, Session& session);








static void print_session(Session& session){
	printf("%s:\n\tobservers: %i\n\tplayers %i/%i\n", session.name.c_str(), session.observers, session.players, session.max_players);
}

static SessionInfo create_or_join_session(int socket_handle, PlayerMode& mode, const std::string& name, int max_players){
	std::cout << active_sessions.size() << std::endl;
	//printf("Entering create_or_join\n");
	if(auto session_found = active_sessions.at(name)){
		auto& session = *session_found;
		if(mode == PLAYER && session.players < session.max_players){
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
		auto& [key, session] = active_sessions.insert({name, {
				name, 
				{socket_handle},
				(mode == OBSERVER) ? 1 : 0,
				(mode == PLAYER) ? 1 : 0, 
				max_players}
				});
		printf("Creating session\n");
		print_session(session);
		return {true, name, mode};
	}else{
		// trying to create session with 0 players, won't fly!
	}

	return {};
}

static void disconnect_from_session(SessionInfo& info){
	if(auto session_found = active_sessions.at(info.name)){
		auto& session = *session_found;
		switch(info.mode){
			case PLAYER: 		session.players--;		break;
			case OBSERVER: 	session.observers--; 	break;
		};

		printf("Disconnecting from session\n");
		print_session(session);

		if(session.players <= 0 && session.observers <= 0){
			printf("Session empty: closing\n");
			active_sessions.erase(info.name);
		}
	}else{
		printf("Trying to disconnect from session %s that does not exist!\n", info.name.c_str());
	}
}

static void send_observer_data(std::vector<int> observers, Session& session){
	if(auto game_found = active_games.at(session.name)){
		auto& game = *game_found;

		BinaryData data;
		serialize_map(data, *game.map);

		for(auto& s : observers){
			tcp_socket::send_all(s, &data[0], data.size());
		}
	}
}

static void start_game(Session& session){
	auto& [name, game] = active_games.insert({session.name, {}});
	// TODO: warning new!!!!
	game.map = new HexagonalMap(3, 20, session.players);

	if(session.observers > 0){
		send_observer_data(session.observer_handles, session);
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
}

struct PlayerData{
};

}
