set(PROJECT tests)
set(SOURCES
		../0x/hex.cpp
		hex_tests.cpp
		memory_tests.cpp
		hex_map_tests.cpp
		crc_tests.cpp
		)

project(${PROJECT})
add_executable(${PROJECT} ${SOURCES})

target_compile_options(${PROJECT} PRIVATE
	-std=c++17 -g
	)

include_directories(${CMAKE_SOURCE_DIR})

#target_link_libraries(${PROJECT}
#	pthread
#	)
