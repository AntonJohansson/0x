#include "hex.hpp"
#include <cmath>

//HexagonalMap::HexagonalMap(int radius, float hex_size, int players)
//	: radius(radius),
//		hex_size(hex_size),
//		hex_width(sqrt(3)*hex_size),
//		hex_height(2*hex_size),
//		players(players){
//	size_t size = (2*radius+1)*(2*radius+1);
//	storage = new HexCell[size];
//
//	// midpoints
//	int mq = radius;
//	int mr = -radius - 1;
//	//int mq = 2*radius+1;
//	//int mr = -radius-1;
//	for(auto& [q, r] : offset_midpoints){
//		q = mq;
//		r = mr;
//
//		std::tie(mq, mr) = axial::rotate_hex(mr, mq, 0, 0);
//	}
//		
//	// might not be necessary
//	for_each([&](int q, int r, HexCell& cell){
//				//printf("%i\n", index(q,r));
//				cell = {q,r,0,-1, false};
//			});
//
//	float angle = 2.0f*M_PI/static_cast<float>(players);
//	for(int i = 0; i < players; i++){
//		auto [q, r] = axial::closest_hex(*this, 400.0f+hex_size*radius*cos(angle*i), 300.0f+hex_size*radius*sin(angle*i));
//		//printf("%i, %i\n", q, r);
//		auto& hex = at(q, r);
//		hex.player_id = i;
//		hex.resources = 10;
//	}
//}

namespace hex{

AxialVec cube_to_axial(const CubeVec& a){
	return {a.x, a.z};
}

CubeVec axial_to_cube(const AxialVec& a){
	// Usually y = -q-r, but the r-axis in our system is flipped
	return {a.q, -a.q+a.r, a.r};
}

int cube_distance(const CubeVec& a, const CubeVec& b){
	return (std::abs(a.x-b.x) + std::abs(a.y-b.y) + std::abs(a.z-b.z))/2;
}

int axial_distance(const AxialVec& a, const AxialVec& b){
	auto c = axial_to_cube(a);
	auto d = axial_to_cube(b);
	return cube_distance(c, d);
}

AxialVec axial_rotate(const AxialVec& point, const AxialVec& center){
	auto c_point  = axial_to_cube(point);
	auto c_center = axial_to_cube(center);

	auto c_delta = c_point - c_center;

	// Rotation clockwise by 60 deg
	// [x y z] -> [-y z x]
	CubeVec v = {-c_delta.y, c_delta.z, c_delta.x};
	v += c_center;
	
	return cube_to_axial(v);
}

std::array<AxialVec, 6> axial_neighbours(const AxialVec& a){
	// possible movement directions
	static std::array<AxialVec, 6> directions = {{
			{+1, 0}, {+1,+1}, { 0,+1},
			{-1, 0}, {-1,-1}, { 0,-1}
	}};

	std::array<AxialVec, 6> neighbours;
	for(int i = 0; i < neighbours.size(); i++){
		neighbours[i] = a + directions[i];
		//if(map.contains(q+v.first, r+v.second)){
		//	neighbours[i] = map.at(q + v.first,r + v.second);
		//}
	}

	return neighbours;
}

}
