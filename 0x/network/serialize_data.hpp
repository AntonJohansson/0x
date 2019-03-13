#pragma once

#include "binary_encoding.hpp"
#include "../hex_map.hpp"

static void serialize_map(BinaryData& data, HexMap& map){
	encode::multiple_integers(data, map.radius, map.players);
	hex_map::for_each(map, [&](HexCell& cell){
				encode::multiple_integers(data, cell.q, cell.r, cell.player_id, cell.resources);
			});
}
