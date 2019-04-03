#include "game_refactor.hpp"
#include "hash_map.hpp"
#include "hex_map.hpp"
#include "murmur_hash3.hpp"
#include "pool_allocator.hpp"
#include <stdio.h>
#include <vector>
#include <chrono>
#include <mutex>
#include <algorithm>

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

	std::vector<ClientId> observers;
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

ObserverDataCallbackFunc observer_data_callback = nullptr;
PlayerDataCallbackFunc player_data_callback = nullptr;
ErrorCallbackFunc error_callback = nullptr;
ConnectedToLobbyCallbackFunc connected_to_lobby_callback = nullptr;

}

static bool lobby_is_open(const Lobby& lobby){
	printf("lobby: %i/%i\n",lobby.number_of_players, lobby.settings.max_number_of_players);
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

static void send_observer_data(Game& game, const std::vector<ClientId>& observers){
	// TODO: this is ridicoulus
	uint32_t radius, player_count, current_turn;
	std::vector<PlayerScores> player_scores;
	std::vector<const HexCell*> map;

	radius = game.map->radius;
	player_count = game.players.size();
	current_turn = game.current_turn;

	HashMap<ClientId, uint32_t> resources;
	hex_map::for_each(*game.map, [&](HexCell& cell){
				map.push_back(&cell);
				if(cell.player_id != -1){
					auto& value = resources[cell.player_id];
					value += cell.resources;
				}
			});

	resources.for_each([&](auto pair){
				player_scores.push_back({pair.key, pair.value});
			});

	for(auto& id : observers){
		observer_data_callback(id, radius, player_count, current_turn, player_scores, map);
	}
}

static void start_new_turn(Game& game){
	//printf("starting new turn\n");

	if(game.current_turn > 0 && ++game.current_player != game.players.end()){
		//printf("-- next player\n");
	}else{
		if(game.current_turn > 0){
			//printf("starting new round");
			hex_map::for_each(*game.map, [](HexCell& cell){
					if(cell.player_id != -1){
						cell.resources++;
					}
				});
		}
		
		game.current_player = game.players.begin();
	}

	game.current_turn++;
	if(game.current_turn > 0){
		game.current_player_begin_turn_time_point = std::chrono::high_resolution_clock::now();
	}

	if(game.observers.size() > 0){
		send_observer_data(game, game.observers);
	}

	std::vector<HexPlayerData> player_map;
	hex_map::for_each(*game.map, [&](HexCell& cell){
				if(cell.player_id == game.current_player->client_id){
					auto& hex_data = player_map.emplace_back(HexPlayerData{&cell, {}});

					auto neighbours = hex::axial_neighbours({cell.q, cell.r});
					for(size_t i = 0; i < hex_data.neighbours.size(); i++){
						auto& n_cell = hex_map::at(*game.map, neighbours[i]);
						hex_data.neighbours[i] = &n_cell;
					}
				}
			});

	// TODO I DONT THINK THIS IS NEEDED
	if(!player_map.empty()){
		player_data_callback(game.current_player->client_id, player_map);
	}else{
		// player got knocked out
	}
}



static void start_game(Lobby& lobby){
	printf("starting new game\n");
	if(auto game_found = active_games.at(lobby.id); game_found){
		printf("-- game already found, restarting\n");
		auto& game = *game_found;

		if(game.map){
			hex_map_allocator.free(game.map);
		}
		active_games.erase(lobby.id);
	}

	auto& game = active_games[lobby.id];
	game.map = hex_map_allocator.alloc(lobby.settings.map_radius, lobby.settings.max_number_of_players,lobby.players);

	printf("-- pushing players\n");
	for(auto& player : lobby.players){
		printf("---- %i\n", player);
		game.players.push_back({player, 10});
	}

	game.observers = lobby.observers;

	if(lobby.number_of_observers){
		// send out data to observers
		send_observer_data(game, lobby.observers);
	}

	start_new_turn(game);
}


//
// Interface to queue requests
//

void disconnect_from_lobby(game::ClientId client_id, game::LobbyId lobby_id){
	std::unique_lock<std::mutex> lock(queue_mutex);
	//TODO: not handling if current_player has client_id
	if(auto game_found = active_games.at(lobby_id); game_found){
		auto& game = *game_found;

		//if(game.current_player->client_id == client_id){
		//	printf("rtucxk\n");
		//	if(++game.current_player == game.players.end()){
		//		game.current_player = game.players.begin();
		//	}
		//}

		game.observers.erase(std::remove(game.observers.begin(), game.observers.end(), lobby_id), game.observers.end());
		game.players.erase(std::remove_if(game.players.begin(), game.players.end(), [&](auto& info){return info.client_id == client_id;}), game.players.end());

		if(game.players.empty()){
			printf("game empty, closing\n");
			hex_map_allocator.free(game.map);
			active_games.erase(lobby_id);
		}
	}

	std::string lobby_name = "";
	active_lobbies.for_each([&](auto& pair){
				if(pair.value.id == lobby_id){
					lobby_name = pair.value.settings.name;
				}
			});

	if(!lobby_name.empty()){
		if(auto lobby_found = active_lobbies.at(lobby_name); lobby_found){
			auto& lobby = *lobby_found;

			size_t before, after;
			before = lobby.observers.size();
			lobby.observers.erase(std::remove(lobby.observers.begin(), lobby.observers.end(), client_id), lobby.observers.end());
			after  = lobby.observers.size();
			if(before != after){
				lobby.number_of_observers--;
			}

			before = lobby.players.size();
			lobby.players.erase(std::remove(lobby.players.begin(), lobby.players.end(), client_id), lobby.players.end());
			after  = lobby.players.size();
			if(before != after){
				lobby.number_of_players--;
			}



			if(lobby.players.empty() && lobby.observers.empty()){
				printf("lobby empty, closing\n");
				active_lobbies.erase(lobby_name);
			}
		}
	}

}

std::vector<std::string> get_lobby_list(){
	std::unique_lock<std::mutex> lock(queue_mutex);
	std::vector<std::string> data;
	active_lobbies.for_each([&](auto& pair){
				data.push_back(pair.key);
			});
	return data;
}

void create_or_join_lobby(ClientId client_id, PlayerMode mode, Settings settings){
	std::unique_lock<std::mutex> lock(queue_mutex);
	lobby_request_queue.push_back({client_id, mode, settings});
	printf("queueing lobby request\n");
}

void commit_player_turn(LobbyId lobby_id, uint32_t amount, int32_t q0, int32_t r0, int32_t q1, int32_t r1){
	std::unique_lock<std::mutex> lock(queue_mutex);
	commit_turn_request_queue.push_back({lobby_id, amount, q0,r0, q1,r1});
}


//
// 
//

void poll(){
	std::unique_lock<std::mutex> lock(queue_mutex);

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

		if(auto game_found = active_games.at(lobby->id); !game_found && !lobby_is_open(*lobby)){
			printf("starting game\n");
			start_game(*lobby);
		}else if(request.client_mode == OBSERVER){
			if(auto game_found = active_games.at(lobby->id); game_found){
				auto& game = *game_found;
				game.observers.push_back(request.client_id);
				send_observer_data(game, {request.client_id});
			}
		}
	}	
	lobby_request_queue.clear();

	active_games.for_each([](auto& pair){
				auto& [lobby_id, game] = pair;
				//printf("\r waiting on player: %i\n", game.current_player->client_id);

				auto dur = std::chrono::high_resolution_clock::now() - game.current_player_begin_turn_time_point;
				//if(dur > std::chrono::milliseconds(500)){
				//	error_callback(game.current_player->client_id, "time limit crossed.");
				//	start_new_turn(game);
				//	// doing this delete fucks up every thing
				//	//game.current_player = game.players.erase(game.current_player);
				//}
			});
	
	for(auto& request : commit_turn_request_queue){
		if(auto game_found = active_games.at(request.lobby_id); game_found){
			auto& game = *game_found;
			auto& cell_transfer_from 	= hex_map::at(*game.map, {request.q0,request.r0});
			auto& cell_transfer_to 		= hex_map::at(*game.map, {request.q1,request.r1});

			// TODO: confirm that transaction actually comes from correct id
			if(game.current_player->client_id != cell_transfer_from.player_id){
				start_new_turn(game);
				continue;
			}

			// TODO: finish the resources transfers
			if(request.amount <= cell_transfer_from.resources){
				cell_transfer_from.resources -= request.amount;
			}else{
				error_callback(game.current_player->client_id, "invalid transaction (" + std::to_string(request.q0) + ", " + std::to_string(request.r0) + ") has " + std::to_string(cell_transfer_from.resources) + ", trying to transfer " + std::to_string(request.amount));
				start_new_turn(game);
				continue;
			}
			
			if(game.current_player->client_id == cell_transfer_from.player_id && cell_transfer_from.player_id != cell_transfer_to.player_id){
				if(cell_transfer_to.resources < request.amount){
					// capture
					cell_transfer_to.resources = request.amount - cell_transfer_to.resources;
					cell_transfer_to.player_id = cell_transfer_from.player_id;
				}else{
					// transfer but not capture
					cell_transfer_to.resources -= request.amount;
				}
			}else{
				// boost
				cell_transfer_to.resources += request.amount;
			}

			// make hex neutral if score (after transfer) is zero
			if(cell_transfer_from.resources == 0){
				cell_transfer_from.player_id = -1;
			}
			if(cell_transfer_to.resources == 0){
				cell_transfer_to.player_id = -1;
			}

			// TODO
			// check for zero resource things
			auto still_in_game = [](HexMap& map, ClientId id){
				uint32_t tiles = 0;
				hex_map::for_each(map, [&](auto& cell){
							if(cell.player_id == id){
								tiles++;
								return;
							}
						});

				return tiles;
			};

			game.players.erase(std::remove_if(game.players.begin(), game.players.end(), [&](auto& info){
						return !still_in_game(*game.map, info.client_id);
						}), game.players.end());
			
			start_new_turn(game);
		}
	}
	commit_turn_request_queue.clear();
}

void set_observer_data_callback(ObserverDataCallbackFunc func){
	observer_data_callback = func;
}

void set_player_data_callback(PlayerDataCallbackFunc func){
	player_data_callback = func;
}

void set_error_callback(ErrorCallbackFunc func){
	error_callback = func;
}

void set_connected_to_lobby_callback(ConnectedToLobbyCallbackFunc func){
	connected_to_lobby_callback = func;
}

}
