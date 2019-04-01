#include "hex.hpp"

HexagonalMap::HexagonalMap(int radius, int players)
	: radius(radius),
		players(players){
	//size_t size = (2*radius+1)*(2*radius+1);
	//storage = new HexCell[size];
	//
	//// might not be necessary
	//for_each([&](int q, int r, HexCell& cell){
	//			//printf("%i\n", index(q,r));
	//			cell = {q,r,0,0};
	//		});

	//float angle = 2.0f*M_PI/static_cast<float>(players);
	//for(int i = 1; i <= players; i++){
	//	auto [q, r] = axial::closest_hex(*this, 400.0f+hex_size*radius*cos(angle*i), 300.0f+hex_size*radius*sin(angle*i));
	//	printf("%i, %i\n", q, r);
	//	auto& hex = at(q, r);
	//	hex.player_id = i;
	//	hex.resources = 10;
	//}
}
