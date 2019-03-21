#include "serialize_data.hpp"
#include "../game.hpp"

void serialize_game_state(BinaryData& data, game::Game& game){
	auto& map = *game.map;
	// Game info
	encode::multiple_integers(data, map.radius, map.players, game.current_turn);

	// Player info
	for(int i = 0; i < map.players; i++){
		encode::multiple_integers(data, i, game.player_scores[i]);
	}

	// encode actual map
	hex_map::for_each(map, [&](HexCell& cell){
				encode::multiple_integers(data, cell.q, cell.r, cell.player_id, cell.resources);
			});
}
