#include "external/catch.hpp"
#include <serialize/binary_encoding.hpp>

TEST_CASE("append ptr", "[binary_encoding]"){
	BinaryData data;
	data.resize(100);
	uint8_t append_data[100];
	for(int i = 0; i < 100; i++){
		data[i] = i;
		append_data[i] = static_cast<uint8_t>(i - '0');
	}

	append(data, append_data, 100);

	REQUIRE(data.size() == 200);
}
