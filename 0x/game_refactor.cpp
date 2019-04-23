#include "game_refactor.hpp"

#include <serialize/murmur_hash3.hpp>
#include <memory/hash_map.hpp>
#include <memory/pool_allocator.hpp>
#include <game/hex_map.hpp>

#include <stdio.h>
#include <vector>
#include <chrono>
#include <mutex>
#include <algorithm>

namespace game{
namespace{

static LobbyId global_lobby_id_counter = 0;

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
	Settings settings;

	std::vector<ClientId> observers;
	std::vector<ClientId> players;
};

struct Game{
	struct PlayerInfo{
		ClientId client_id;
		uint32_t score;
	};

	bool restart_on_win = false;

	uint32_t current_turn = 0;
	uint32_t max_players = 0;

	std::vector<ClientId> observers;
	std::vector<PlayerInfo> players;
	std::vector<PlayerInfo>::iterator current_player;
	std::chrono::high_resolution_clock::time_point current_player_begin_turn_time_point;

	HexMap* map = nullptr;
};

static std::mutex queue_mutex;
static std::vector<LobbyRequest> lobby_request_queue;
static std::vector<CommitPlayerTurnRequest> commit_turn_request_queue;

static HashMap<std::string, Lobby> active_lobbies;
static HashMap<LobbyId, Game> active_games;

struct ClientInfo{
	Lobby* lobby = nullptr;
	Game* game = nullptr;
	PlayerMode connected_as = NONE;
};
static HashMap<ClientId, ClientInfo> client_info;

static PoolAllocator<HexMap> hex_map_allocator;

static ObserverDataCallbackFunc observer_data_callback = nullptr;
static PlayerDataCallbackFunc player_data_callback = nullptr;
static ErrorCallbackFunc error_callback = nullptr;
static ConnectedToLobbyCallbackFunc connected_to_lobby_callback = nullptr;

}

static bool lobby_is_open(const Lobby& lobby){
	return lobby.players.size() < lobby.settings.max_number_of_players;
}

static bool valid_lobby_request(const Lobby& lobby, const LobbyRequest& request){
	if(request.client_mode == OBSERVER && request.settings.name.size() > 0){
		return true;
	}else if(request.client_mode == PLAYER && lobby_is_open(lobby)){
		return true;
	}else{
		error_callback(request.client_id, "lobby request invalid (mode: " + std::to_string(request.client_mode) + ", " + std::to_string(lobby.players.size()) + "/" + std::to_string(lobby.settings.max_number_of_players) + ")");
	}

	return false;
}

static void send_observer_data(Game& game, const std::vector<ClientId>& observers){
	// TODO: this is ridicoulus
	uint32_t radius, player_count, max_players, current_turn;
	std::vector<PlayerScores> player_scores;
	std::vector<const HexCell*> map;

	radius = game.map->radius;
	player_count = game.players.size();
	max_players = game.max_players;
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
		observer_data_callback(id, radius, player_count, max_players, current_turn, player_scores, map);
	}
}

static void start_new_turn(Game& game){
	//printf("\tstarting new turn\n");

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

	//printf("getting player map for %i\n", game.current_player->client_id);
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
	//printf("map size: %zu\n", player_map.size());
	//printf("player size: %zu\n", game.players.size());

	// TODO I DONT THINK THIS IS NEEDED
	if(!player_map.empty()){
		player_data_callback(game.current_player->client_id, player_map);
	}else{
		// player got knocked out
	}
}



static void start_game(Lobby& lobby){
	// remove old game
	if(auto game_found = active_games.at(lobby.id); game_found){
		auto& game = *game_found;

		if(game.map){
			hex_map_allocator.free(game.map);
		}
		active_games.erase(lobby.id);
	}

	auto& game = active_games[lobby.id];
	game.map = hex_map_allocator.alloc(lobby.settings.map_radius, lobby.settings.max_number_of_players,lobby.players);

	game.max_players = lobby.players.size();
	game.restart_on_win = lobby.settings.restart_on_win;
	printf("restart: %i\n", (int)game.restart_on_win);

	for(auto& player : lobby.players){
		// 10 = starting score
		game.players.push_back({player, 10});
		client_info[player].game = &game;
	}

	game.observers = lobby.observers;
	for(auto& observer : lobby.observers){
		client_info[observer].game = &game;
	}


	if(lobby.observers.empty()){
		// send out data to observers
		send_observer_data(game, lobby.observers);
	}

	start_new_turn(game);
}


//
// Interface to queue requests
//

void disconnect_if_connected(game::ClientId client_id){
	if(auto found = client_info.at(client_id)){
		auto& info = *found;
		switch(info.connected_as){
			case PLAYER:
				if(auto game = info.game){
					game->players.erase(
							std::remove_if(	game->players.begin(), 
															game->players.end(), 
															[&](auto& info){
																return info.client_id == client_id;
															}), 
							game->players.end());
				}
				if(auto lobby = info.lobby){
					lobby->players.erase(
							std::remove(lobby->players.begin(), 
													lobby->players.end(), 
													client_id), 
							lobby->players.end());
				}
				break;
			case OBSERVER:
				if(auto game = info.game){
					game->observers.erase(
							std::remove(game->observers.begin(), 
													game->observers.end(), 
													client_id), 
							game->observers.end());
				}
				if(auto lobby = info.lobby){
					lobby->observers.erase(
							std::remove(lobby->observers.begin(), 
													lobby->observers.end(), 
													client_id), 
							lobby->observers.end());

				}
				break;
		}

		// close game/lobby if neeeded 
		if(info.game && info.game->players.empty() && info.game->observers.empty()){
			hex_map_allocator.free(info.game->map);
			active_games.erase(info.lobby->id);
		}
		if(info.lobby && info.lobby->players.empty() && info.lobby->observers.empty()){
			std::string lobby_name = "";
			active_lobbies.for_each([&](auto& pair){
						if(pair.value.id == info.lobby->id){
							lobby_name = pair.key;
						}
					});
			if(lobby_name != ""){
				active_lobbies.erase(lobby_name);
			}
		}

		info.game = nullptr;
		info.lobby = nullptr;
		info.connected_as = NONE;
	}
}

void disconnect_from_lobby(game::ClientId client_id, game::LobbyId lobby_id){
	std::unique_lock<std::mutex> lock(queue_mutex);
	disconnect_if_connected(client_id);
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

		disconnect_if_connected(request.client_id);
		if(auto lobby_found = active_lobbies.at(request.settings.name)){
			lobby = &*lobby_found;
			if(!valid_lobby_request(*lobby, request)){
				continue;
			}
		}else{
			//printf("creating new lobby\n");
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
				lobby->observers.push_back(request.client_id);
				client_info.insert({request.client_id, {lobby, nullptr, OBSERVER}});
				break;
			case PLAYER:
				lobby->players.push_back(request.client_id);
				client_info.insert({request.client_id, {lobby, nullptr, PLAYER}});
				break;
			// no need to check default case since they 
			// will already be checked.
		}

		connected_to_lobby_callback(request.client_id, lobby->id);

		if(auto game_found = active_games.at(lobby->id); !game_found && !lobby_is_open(*lobby)){
			//printf("starting game\n");
			start_game(*lobby);
		}else if(request.client_mode == OBSERVER){
			if(auto game_found = active_games.at(lobby->id)){
				auto& game = *game_found;
				game.observers.push_back(request.client_id);
				client_info[request.client_id].game = &game;
				send_observer_data(game, {request.client_id});
			}
		}
	}	
	lobby_request_queue.clear();

	// -----------------------------------------------
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
	
	// -----------------------------------------------
	for(auto& request : commit_turn_request_queue){
		//printf("\tprocessing turn request from \n");
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
			// ...
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

			// TODO: LOL
			size_t current_player_index = game.current_player - game.players.begin();
			size_t before = game.players.size();
			game.players.erase(std::remove_if(game.players.begin(), game.players.end(), [&](auto& info){
						if(!still_in_game(*game.map, info.client_id)){
							return true;
						}
						return false;
						}), game.players.end());
			if(game.players.size() != before){
				if(current_player_index > game.players.size()){
					game.current_player = game.players.begin();
				}else{
					game.current_player = game.players.begin() + current_player_index-1;
				}
			}

			if(game.players.size() == 1 && game.restart_on_win){
				// AWWWWWWWWWWWWWWWWWWHG
				Lobby* lobby = nullptr;
				active_lobbies.for_each([&](auto& pair){
							if(pair.value.id == request.lobby_id){
								lobby = &pair.value;
								return;
							}
						});

				if(lobby){
					start_game(*lobby);
				}
			}else{
				start_new_turn(game);
			}
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
