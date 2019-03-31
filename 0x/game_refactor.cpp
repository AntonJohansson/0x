#include "game_refactor.hpp"
#include "hash_map.hpp"
#include "hex_map.hpp"
#include "murmur_hash3.hpp"
#include "pool_allocator.hpp"
#include <vector>
#include <chrono>
#include <mutex>

namespace game{
namespace{

LobbyId global_lobby_id_counter = 0;

struct LobbyRequest{
	ClientId client_id = 0;
	PlayerMode client_mode = NONE;
	Settings settings;
};

struct CommitPlayerTurnRequest{
	LobbyId lobby_id;
	uint32_t amount;
	uint32_t q0, r0;
	uint32_t q1, r1;
};





struct Game;
struct Lobby{
	LobbyId id;
	uint32_t number_of_players = 0;
	uint32_t number_of_observers = 0;
	Settings settings;

	std::vector<ClientId> observers;
	std::vector<ClientId> players;
};

struct Game{
	struct PlayerInfo{
		ClientId client_id;
		uint32_t score;
	};

	uint32_t current_turn = 0;

	std::vector<PlayerInfo> players;
	std::vector<PlayerInfo>::iterator current_player;
	std::chrono::high_resolution_clock::time_point current_player_begin_turn_time_point;

	HexMap* map = nullptr;
};

std::mutex queue_mutex;
std::vector<LobbyRequest> lobby_request_queue;
std::vector<CommitPlayerTurnRequest> commit_turn_request_queue;

HashMap<std::string, Lobby> active_lobbies;
HashMap<LobbyId, Game> active_games;

PoolAllocator<HexMap> hex_map_allocator;

ErrorCallbackFunc error_callback = nullptr;
ConnectedToLobbyCallbackFunc connected_to_lobby_callback = nullptr;

}

static bool lobby_is_open(const Lobby& lobby){
	return lobby.number_of_players < lobby.settings.max_number_of_players;
}

static bool valid_lobby_request(const Lobby& lobby, const LobbyRequest& request){
	if(request.client_mode == OBSERVER && request.settings.name.size() > 0){
		return true;
	}else if(request.client_mode == PLAYER && lobby_is_open(lobby)){
		return true;
	}else{
		error_callback(request.client_id, "lobby request invalid (mode: " + std::to_string(request.client_mode) + ", " + std::to_string(lobby.number_of_players) + "/" + std::to_string(lobby.settings.max_number_of_players) + ")");
	}

	return false;
}

static void start_game(Lobby& lobby){
	if(auto game_found = active_games.at(lobby.id); game_found){
		auto& game = *game_found;

		if(game.map){
			hex_map_allocator.free(game.map);
		}
		active_games.erase(lobby.id);
	}

	auto& game = active_games[lobby.id];
	game.map = hex_map_allocator.alloc(lobby.settings.map_radius, lobby.settings.max_number_of_players);

	for(auto& player : lobby.players){
		game.players.push_back({player, 10});
	}

	game.current_player = game.players.begin();

	if(lobby.number_of_observers){
		// send out data to observers
	}
}




//
// Interface to queue requests
//

void create_or_join_lobby(ClientId client_id, PlayerMode mode, Settings settings){
	std::unique_lock<std::mutex> lock(queue_mutex);
	lobby_request_queue.push_back({client_id, mode, settings});
	printf("queueing lobby request\n");
}

void commit_player_turn(LobbyId lobby_id, uint32_t amount, uint32_t q0, uint32_t r0, uint32_t q1, uint32_t r1){
	std::unique_lock<std::mutex> lock(queue_mutex);
	commit_turn_request_queue.push_back({lobby_id, amount, q0,r0, q1,r1});
}





//
// 
//

void poll(){
	for(auto& request : lobby_request_queue){
		Lobby* lobby = nullptr;

		// Fetch or create the lobby,
		// then check if the request is valid.
		if(auto lobby_found = active_lobbies.at(request.settings.name); lobby_found){
			printf("joining lobby\n");
			lobby = &*lobby_found;
			if(!valid_lobby_request(*lobby, request)){
				continue;
			}
		}else{
			printf("creating new lobby\n");
			Lobby new_lobby{
				.id = ++global_lobby_id_counter,
				.settings = request.settings
			};

			if(valid_lobby_request(new_lobby, request)){
				auto& pair = active_lobbies.insert({request.settings.name, new_lobby});
				lobby = &pair.value;
			}else{
				continue;
			}
		}

		switch(request.client_mode){
			case OBSERVER:
				lobby->number_of_observers++;
				lobby->observers.push_back(request.client_id);
				break;
			case PLAYER:
				lobby->number_of_players++;
				lobby->players.push_back(request.client_id);
				break;
			// no need to check default case since they 
			// will already be checked.
		}

		connected_to_lobby_callback(request.client_id, lobby->id);

		if(!lobby_is_open(*lobby)){
			printf("starting game\n");
			start_game(*lobby);
		}
	}	
	lobby_request_queue.clear();

	active_games.for_each([](auto& pair){
				auto& [lobby_id, game] = pair;

				auto dur = std::chrono::high_resolution_clock::now() - game.current_player_begin_turn_time_point;
				if(dur > std::chrono::milliseconds(500)){
					error_callback(game.current_player->client_id, "time limit crossed.");
					game.current_player = game.players.erase(game.current_player);
				}
			});
	
	for(auto& request : commit_turn_request_queue){
		if(auto game_found = active_games.at(request.lobby_id); game_found){
			auto& game = *game_found;
			auto& cell_transfer_from 	= hex_map::at(*game.map, {request.q0,request.r0});
			auto& cell_transfer_to 		= hex_map::at(*game.map, {request.q1,request.r1});

			// TODO: finish the resources transfers
		}
	}
}






void set_error_callback(ErrorCallbackFunc func){
	error_callback = func;
}

void set_connected_to_lobby_callback(ConnectedToLobbyCallbackFunc func){
	connected_to_lobby_callback = func;
}

}
