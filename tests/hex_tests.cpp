#define CATCH_CONFIG_MAIN
#include "external/catch.hpp"
#include <game/hex.hpp>

using namespace hex;

TEST_CASE("axial op. overloads", "[hex]"){
	AxialVec a =  {4,3}, b = {3, -6};
	REQUIRE((a+b)		== AxialVec{7,-3});
	REQUIRE((a-b)		== AxialVec{1, 9});
	REQUIRE((a+=b) 	== AxialVec{7,-3});
	REQUIRE((a-=b) 	== AxialVec{4, 3});
}

TEST_CASE("cube op. overloads", "[hex]"){
	CubeVec a =  {4,3}, b = {3, -6};
	REQUIRE((a+b)		== CubeVec{7,-3});
	REQUIRE((a-b)		== CubeVec{1, 9});
	REQUIRE((a+=b) 	== CubeVec{7,-3});
	REQUIRE((a-=b) 	== CubeVec{4, 3});
}

TEST_CASE("cube/axial conversion", "[hex]"){
	AxialVec start = {5, 1};
	auto c = hex::axial_to_cube(start);
	auto a = hex::cube_to_axial(c);
	REQUIRE(c == CubeVec{start.q, -start.q+start.r, start.r});
	REQUIRE(a == start);
}

TEST_CASE("axial distance", "[hex]"){
	AxialVec c = {0,0};
	REQUIRE(hex::axial_distance({0,0}, c) == 0);
	REQUIRE(hex::axial_distance({1,0}, c) == 1);
	REQUIRE(hex::axial_distance({2,0}, c) == 2);
	REQUIRE(hex::axial_distance({3,0}, c) == 3);
}

TEST_CASE("axial 60 deg rotate", "[hex]"){
	AxialVec start 	= {5, -3};
	AxialVec point 	= start;
	AxialVec center =  {3, 2};
	point = hex::axial_rotate(point, center);
	point = hex::axial_rotate(point, center);
	point = hex::axial_rotate(point, center);
	point = hex::axial_rotate(point, center);
	point = hex::axial_rotate(point, center);
	point = hex::axial_rotate(point, center);
	REQUIRE(point == start);
}

TEST_CASE("axial neighbours", "[hex]"){
	static std::array<AxialVec, 6> directions = {{
			{+1, 0}, {+1,+1}, { 0,+1},
			{-1, 0}, {-1,-1}, { 0,-1}
	}};

	AxialVec p = {5, 5};
	auto n = hex::axial_neighbours(p);
	for(int i = 0; i < n.size(); i++){
		REQUIRE(n[i] == p + directions[i]);
	}
}
