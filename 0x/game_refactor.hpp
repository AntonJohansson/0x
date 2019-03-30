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
	uint32_t max_players = 0;
	uint32_t map_radius = 0;
};

// ways to interact with running games
extern void create_or_join_session(int client_id, PlayerMode mode, Settings settings);
extern void commit_player_turn();

}
