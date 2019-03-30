#include "game_refactor.hpp"
#include "hash_map.hpp"
#include "hex_map.hpp"
#include <vector>
#include <chrono>

namespace game{
namespace{

struct SessionRequest{
	int client_id = 0;
	PlayerMode mode = NONE;
	Settings settings;
};

struct Game;
struct Session{
	uint32_t number_of_players = 0;
	uint32_t number_of_observers = 0;
	Settings settings;

	Game* game = nullptr;
};

struct Game{
	struct PlayerInfo{
		uint32_t score;
		uint32_t number_of_tiles;
	};

	uint32_t current_turn = 0;
	std::chrono::high_resolution_clock::time_point current_player_begin_turn_time_point;

	HexMap* map = nullptr;
};

std::vector<SessionRequest> session_request_queue;

HashMap<Lobby> active_lobbies;
HashMap<Game> active_games;

PoolAllocator<HexMap> hex_map_allocator;

}

void create_or_join_lobby(int client_id, PlayerMode mode, Settings settings){
	session_request_queue.push_back({client_id, mode, settings});
}

void commit_player_turn(){

}

}
