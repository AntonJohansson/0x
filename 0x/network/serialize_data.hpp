#pragma once

#include "binary_encoding.hpp"
#include "../hex.hpp"

void serialize_map(BinaryData& data, HexagonalMap& map){
	encode::multiple_integers(data, map.radius, map.players);
	map.for_each([&](int q, int r, HexCell& cell){
				encode::multiple_integers(data, q, r, cell.player_id, cell.resources);
			});
}
