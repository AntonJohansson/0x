#include "external/catch.hpp"
#include "0x/hex_map.hpp"
#include <iostream>

TEST_CASE("midpoints", "[hex_map]"){
	HexMap map(2, 3);
	REQUIRE(map.offset_midpoints[0] == hex::AxialVec{ 2,-3});
	REQUIRE(map.offset_midpoints[1] == hex::AxialVec{ 5, 2});
	REQUIRE(map.offset_midpoints[2] == hex::AxialVec{ 3, 5});
	REQUIRE(map.offset_midpoints[3] == hex::AxialVec{-2, 3});
	REQUIRE(map.offset_midpoints[4] == hex::AxialVec{-5,-2});
	REQUIRE(map.offset_midpoints[5] == hex::AxialVec{-3,-5});
}




