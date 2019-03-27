#include "external/catch.hpp"
#include "0x/crc/crc32.hpp"

TEST_CASE("crc(1234567989)", "[crc]"){
	char* buffer = "123456789";
	REQUIRE(buffer_crc32(reinterpret_cast<uint8_t*>(buffer), 9) == 0xe3069283);

}
