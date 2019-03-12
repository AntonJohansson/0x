#pragma once

#include <string>
#include <stdint.h>
#include <functional>
#include <array>
#include <cmath>
#include <assert.h>

// Hexagons are point-tipped!

struct HexVec{
	int q, r;
};

struct HexCell{
	int q = 0, r = 0;
	int resources = 0;
	int player_id = -1;
	bool selected = false;
};

namespace axial{
static int distance(int q1, int r1, int q2, int r2);
}

struct HexagonalMap{
	const int radius;

	const float hex_size    = 0;
	const float hex_width   = 0;
	const float hex_height  = 0;

	int players = 0;
	HexCell* storage = nullptr;

	// used in wrapping
	std::array<HexVec, 6> offset_midpoints;

	HexagonalMap(int radius, float hex_size, int players);

	inline size_t index(int q, int r){
		return (r+radius) + (q+radius)*(2*radius+1);
	}

	inline bool contains(int q, int r){
		auto i = index(q,r);
		//printf("%i\n", i);
		return (i < (2*radius+1)*(2*radius+1));//&& storage[i].player_id > 0);
	}

	inline HexCell& at(int q, int r){
		printf("q,r: {%i, %i}\n", q, r);
		if(axial::distance(q,r, 0,0) > radius){
			printf("q,r outside radius %i vs %i\n", axial::distance(q, r, 0, 0), radius);
			int smallest_distance = 2*radius;
			int smallest_i = 0;
			for(int i = 0; i < offset_midpoints.size(); i++){
				auto d = axial::distance(q,r,offset_midpoints[i].q, offset_midpoints[i].r);
				if(d < smallest_distance){
					smallest_i = i;
					smallest_distance = d;
				}
			}

			q -= offset_midpoints[smallest_i].q;
			r -= offset_midpoints[smallest_i].r;

			printf("new q, r: %i, %i\n", q, r);
		}

		assert(contains(q, r));
		HexCell& h = storage[index(q,r)];
		//assert(h.player_id != 0);
		return h;
	}

	inline void for_each(std::function<void(int,int,HexCell&)> func){
		for (int q = -radius; q <= radius; q++){
			int r1 = std::max(-radius, q - radius);
			int r2 = std::min( radius, q + radius);
			for (int r = r1; r <= r2; r++){
				func(q, r, storage[index(q,r)]);
			}
		}
	}

	~HexagonalMap(){
		delete[] storage;
		storage = nullptr;
	}
};

//// TODO: Ridiculous amount of string copies!
//static std::string serialize_map(HexagonalMap& map){
//	std::string result;
//	
//	result += "[MAP_INFO]" + std::to_string(map.radius) + "," + std::to_string(map.players) + ";";
//	result += "[MAP_DATA]";
//	map.for_each([&result](int q, int r, HexCell& cell){
//				result += std::to_string(q) + "," + std::to_string(r) + "," + std::to_string(cell.player_id) + "," + std::to_string(cell.resources) + ";";
//			});
//
//	return result;
//}

static std::array<HexCell,6> hex_neighbours(HexagonalMap& map, int q, int r){
	// possible movement directions
	using hex_vec = std::pair<int,int>;
	static std::array<hex_vec, 6> directions = {{
		{+1, 0}, {+1,+1}, { 0,+1},
		{-1, 0}, {-1,-1}, { 0,-1}
	}};

	std::array<HexCell, 6> neighbours;
	for(int i = 0; i < neighbours.size(); i++){
		auto v = directions[i];
		if(map.contains(q+v.first, r+v.second)){
			neighbours[i] = map.at(q + v.first,r + v.second);
		}
	}

	return neighbours;
}

// Coordinate system
namespace axial{

constexpr float pi_over_six = M_PI/6.0f;
// unit vectors
constexpr float x1 = 1.0f;
constexpr float x2 = 0.0f;
// cos/sin not consexpr
//const float y1 = cos(pi_over_six)*cos(pi_over_six*4);
//const float y2 = cos(pi_over_six)*sin(pi_over_six*4);
// cos(pi/6)  = 0.866025403784438646763723170752936183471402626905190314
// cos(4pi/6) = -0.5
// sin(4pi/6) = 0.866025403784438646763723170752936183471402626905190314
constexpr float y1 = 0.8660254f*(-0.5f);
constexpr float y2 = 0.8660254f*0.8660254;

static std::pair<float, float> to_screen(HexagonalMap& map, int q, int r){
	return {r*2*map.hex_size*axial::y1 + q*map.hex_width*axial::x1, r*2*map.hex_size*axial::y2};
}

static std::pair<int, int> from_screen(HexagonalMap& map, float x, float y){
	return {0,0};
}


static std::pair<int,int> hex_round(const float qd, const float rd){
	// convert axial -> cube coordinates
	float sd = - qd - rd;
	
	int q = std::round(qd);
	int r = std::round(rd);
	int s = std::round(sd);

	float q_diff = std::abs(q - qd);
	float r_diff = std::abs(r - rd);
	float s_diff = std::abs(s - sd);

	if(q_diff > r_diff && q_diff > s_diff){
		q = -r-s;
	}else if(r_diff > s_diff){
		r = -q-s;
	}else{
		s = -q-r;
	}

	return {q,r};
}

//
// (a  b)
// (c  d)
//
// 1/(a*d - b*c) * ( d -b)
//                 (-c  a)
//
// 1/(a*d - b*c) * ( d -b)(x)
//                 (-c  a)(y)
//
// 1/(a*d - b*c) * ( dx-by)
//                 (-cx+ay)

static std::pair<int,int> closest_hex(HexagonalMap& map, float x, float y){
	y = 600.0f - y;
	x -= 800.0f/2.0f;
	y -= 600.0f/2.0f;
	x /= 1.05f;
	y /= 1.05f;
	x /= map.hex_width;
	y /= map.hex_height;

	auto a = axial::x1;
	auto b = axial::y1;
	auto c = axial::x2;
	auto d = axial::y2;

	auto det = a*d - b*c;
	auto new_x =  d*x - b*y;
	auto new_y = -c*x + a*y;
	new_x *= 1/det;
	new_y *= 1/det;

	return hex_round(new_x, new_y);
}

static std::tuple<int,int,int> to_cube(int q, int r){
	// Usually y = -q-r, but the r-axis in our system is flipped
	return {q, -q+r, r};
}

static int cube_distance(int x1, int y1, int z1, int x2, int y2, int z2){
	return (std::abs(x1-x2) + std::abs(y1-y2) + std::abs(z1-z2))/2;
}

static int distance(int q1, int r1, int q2, int r2){
	auto [x1,y1,z1] = to_cube(q1, r1);
	auto [x2,y2,z2] = to_cube(q2, r2);
	return cube_distance(x1, y1, z1, x2, y2, z2);
}

static std::pair<int,int> rotate_hex(int qP, int rP, int qC, int rC){
	auto [xP, yP, zP] = to_cube(qP, rP);
	auto [xC, yC, zC] = to_cube(qC, rC);

	int dx = xP - xC;
	int dy = yP - yC;
	int dz = zP - zC;

	// Rotation clockwise by 60 deg
	int x = -dz;
	int y = -dx;
	int z = -dy;

	x += xC;
	y += yC;
	z += zC;

	// cube to axial
	return {x, z};
}

}
