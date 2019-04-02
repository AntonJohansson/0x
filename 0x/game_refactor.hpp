#pragma once

#include <stdint.h>
#include <string>
#include <array>
#include <vector>

struct HexCell;

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

extern void disconnect_from_lobby(game::ClientId client_id, game::LobbyId lobby_id);
extern std::vector<std::string> get_lobby_list();
extern void create_or_join_lobby(ClientId client_id, PlayerMode mode, Settings settings);
extern void commit_player_turn(LobbyId lobby_id, uint32_t amount, int32_t q0, int32_t r0, int32_t q1, int32_t r1);

extern void poll();



// 
// 
//


struct HexPlayerData{
	const HexCell* hex;
	std::array<const HexCell*, 6> neighbours;
};

struct PlayerScores{
	ClientId id;
	uint32_t resources;
};

using ErrorCallbackFunc = void(*)(ClientId, const std::string&);
using ObserverDataCallbackFunc = void(*)(ClientId, 
		uint32_t map_radius, 
		uint32_t player_count,
		uint32_t current_turn,
		std::vector<PlayerScores> player_scores,
		std::vector<const HexCell*> map
		// player scores
		// data in the form of (q,r,id,amount)
		);
using PlayerDataCallbackFunc = void(*)(ClientId,
		std::vector<HexPlayerData> player_map
		// data in the form of (q,r,id,amount, (...neighbours...))
		);
using ConnectedToLobbyCallbackFunc = void(*)(ClientId, LobbyId);

extern void set_error_callback(ErrorCallbackFunc func);
extern void set_observer_data_callback(ObserverDataCallbackFunc func);
extern void set_player_data_callback(PlayerDataCallbackFunc func);
extern void set_connected_to_lobby_callback(ConnectedToLobbyCallbackFunc func);

}
