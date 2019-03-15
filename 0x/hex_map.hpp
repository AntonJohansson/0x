#pragma once

#include "hex.hpp"
#include <array>
#include <functional>
#include <assert.h>

struct HexCell{
	int q = 0, r = 0;
	int resources = 0;
	int player_id = -1;
};

struct HexMap;
struct HexCell;
namespace hex_map{
void for_each(const HexMap& map, std::function<void(HexCell&)> func);
HexCell& at(const HexMap& map, const hex::AxialVec& v);
}

struct HexMap{
	int radius = 0;
	int players = 0;
	std::array<hex::AxialVec, 6> offset_midpoints;

	HexCell* storage = nullptr;

	HexMap(int r, int p)
		: radius(r), players(p) {
		// TODO: big nono
		size_t size = (2*r+1)*(2*r+1);
		storage = new HexCell[size];

		// midpoints
		hex::AxialVec m = {radius, -radius-1};
		for(auto& v : offset_midpoints){
			v = m;
			m = hex::axial_rotate(m, {0,0});
		}

		// intialising start positions
		hex::AxialVec start = {radius-1, 0};
		for(int i = 0; i < players; i++){
			auto& cell = hex_map::at(*this, start);
			cell.player_id = i;
			cell.resources = 10;

			start = hex::axial_rotate(start, {0,0});
		}
	}

	~HexMap(){
		delete[] storage;
	}
};

namespace hex_map{
inline size_t index(const HexMap& map, const hex::AxialVec& v){
	return (v.r + map.radius) + (v.q+map.radius)*(2*map.radius + 1);
}

inline bool contains(const HexMap& map, const hex::AxialVec& v){
		auto i = index(map, v);
		return i < (2*map.radius+1)*(2*map.radius+1);
}

inline hex::AxialVec wrap_axial(const HexMap& map, const hex::AxialVec& v){
	hex::AxialVec ret_v = v;

	if(hex::axial_distance(v, {0,0}) > map.radius){
		int smallest_distance = map.radius+1;
		int smallest_index = 0;
		for(int i = 0; i < map.offset_midpoints.size(); i++){
			auto d = hex::axial_distance(v, map.offset_midpoints[i]);
			if(d < smallest_distance){
				smallest_distance = d;
				smallest_index = i;
			}
		}

		ret_v -= map.offset_midpoints[smallest_index];
	}

	return ret_v;
}

inline HexCell& at(const HexMap& map, const hex::AxialVec& v){
	auto new_v = wrap_axial(map, v);
	assert(contains(map, new_v));
	return map.storage[index(map, new_v)];
}

inline void for_each(const HexMap& map, std::function<void(HexCell&)> func){
	for (int q = -map.radius; q <= map.radius; q++){
		int r1 = std::max(-map.radius, q - map.radius);
		int r2 = std::min( map.radius, q + map.radius);
		for (int r = r1; r <= r2; r++){
			// TODO: this is bad
			auto& cell = map.storage[index(map, {q,r})];
			cell.q = q;
			cell.r = r;

			func(cell);
		}
	}
}


}


