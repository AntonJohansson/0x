#pragma once

#include "binary_encoding.hpp"
//#include "../hex_map.hpp"

inline void encode_frame_length(BinaryData& data){
	// TODO: a lot of copies/allocs occur here
	BinaryData temp;
	encode::u32(temp, data.size());

	append(temp, data);
	data = temp;
}

namespace game{struct Game;}
extern void serialize_game_state(BinaryData& data, game::Game& game);
