#include "game.hpp"

namespace game{

HashMap<std::string, Session> active_sessions;
HashMap<std::string, Game> active_games;
PoolAllocator<HexMap> map_allocator;
std::vector<CompleteTurnData> complete_turn_data;

}
