#pragma once

#include <stdint.h>
#include <string>

namespace game{

enum PlayerMode{
	NONE,
	OBSERVER,
	PLAYER
};

struct Settings{
	std::string name{"default"};
	uint32_t max_number_of_players = 0;
	uint32_t map_radius = 0;
};

using LobbyId = uint64_t;
using ClientId = uint32_t;

// 
// Interfact to queue game requests
//

extern void create_or_join_lobby(ClientId client_id, PlayerMode mode, Settings settings);
extern void commit_player_turn(LobbyId lobby_id, uint32_t amount, uint32_t q0, uint32_t r0, uint32_t q1, uint32_t r1);

extern void poll();



// 
// 
//

using ErrorCallbackFunc = void(*)(ClientId, const std::string&);
using ObserverDataCallbackFunc = void(*)(ClientId, 
		uint32_t map_radius, 
		uint32_t player_count,
		uint32_t current_turn
		// player scores
		// data in the form of (q,r,id,amount)
		);
using PlayerDataCallbackFunc = void(*)(ClientId
		// data in the form of (q,r,id,amount, (...neighbours...))
		);
using ConnectedToLobbyCallbackFunc = void(*)(ClientId, LobbyId);

extern void set_error_callback(ErrorCallbackFunc func);
extern void set_observer_data_callback();
extern void set_player_data_callback();
extern void set_connected_to_lobby_callback(ConnectedToLobbyCallbackFunc func);

}
