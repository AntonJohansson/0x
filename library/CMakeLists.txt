set(VERSION 1)
set(PROJECT library)
set(SOURCES
	game/hex.cpp
	network/socket_connection.cpp
	network/socket.cpp
	serialize/serialize_data.cpp
	)
set(INSTALL_HEADERS
	crc/crc32.hpp
	game/hex.hpp
	game/hex_map.hpp
	game/packet.hpp
	io/poll_set.hpp
	memory/hash_map.hpp
	memory/pool_allocator.hpp
	memory/stack_allocator.hpp
	network/socket_connection.hpp
	network/socket.hpp
	other/number_types.hpp
	other/string_helper.hpp
	serialize/binary_encoding.hpp
	serialize/murmur_hash3.hpp
	serialize/serialize_data.hpp
	)

project(${PROJECT} VERSION 1 DESCRIPTION "library used for hexagon servers/clients")
add_library(${PROJECT} SHARED ${SOURCES})

set_target_properties(${PROJECT} PROPERTIES VERSION ${VERSION})
target_include_directories(${PROJECT} PRIVATE .)

target_compile_options(${PROJECT} PRIVATE
	-std=c++17 -g
	)

target_link_libraries(${PROJECT}
	pthread
	)

add_custom_command(TARGET ${PROJECT} POST_BUILD COMMAND 
	rm -rf ${CMAKE_BINARY_DIR}/${PROJECT}/crc ${CMAKE_BINARY_DIR}/${PROJECT}/game ${CMAKE_BINARY_DIR}/${PROJECT}/io ${CMAKE_BINARY_DIR}/${PROJECT}/memory ${CMAKE_BINARY_DIR}/${PROJECT}/network ${CMAKE_BINARY_DIR}/${PROJECT}/other ${CMAKE_BINARY_DIR}/${PROJECT}/serialize && 
	mkdir -p ${CMAKE_BINARY_DIR}/${PROJECT}/crc 				&& cp ${CMAKE_CURRENT_SOURCE_DIR}/crc/*.hpp 			${CMAKE_BINARY_DIR}/${PROJECT}/crc/ &&
	mkdir -p ${CMAKE_BINARY_DIR}/${PROJECT}/game 				&& cp ${CMAKE_CURRENT_SOURCE_DIR}/game/*.hpp 			${CMAKE_BINARY_DIR}/${PROJECT}/game/ &&
	mkdir -p ${CMAKE_BINARY_DIR}/${PROJECT}/io 					&& cp ${CMAKE_CURRENT_SOURCE_DIR}/io/*.hpp 				${CMAKE_BINARY_DIR}/${PROJECT}/io/ &&
	mkdir -p ${CMAKE_BINARY_DIR}/${PROJECT}/memory 			&& cp ${CMAKE_CURRENT_SOURCE_DIR}/memory/*.hpp 		${CMAKE_BINARY_DIR}/${PROJECT}/memory/ &&
	mkdir -p ${CMAKE_BINARY_DIR}/${PROJECT}/network 		&& cp ${CMAKE_CURRENT_SOURCE_DIR}/network/*.hpp 	${CMAKE_BINARY_DIR}/${PROJECT}/network/ &&
	mkdir -p ${CMAKE_BINARY_DIR}/${PROJECT}/other 			&& cp ${CMAKE_CURRENT_SOURCE_DIR}/other/*.hpp 		${CMAKE_BINARY_DIR}/${PROJECT}/other/ && 
	mkdir -p ${CMAKE_BINARY_DIR}/${PROJECT}/serialize 	&& cp ${CMAKE_CURRENT_SOURCE_DIR}/serialize/*.hpp ${CMAKE_BINARY_DIR}/${PROJECT}/serialize/
	)

set_target_properties(${PROJECT} PROPERTIES PUBLIC_HEADER ${INSTALL_HEADERS})
include(GNUInstallDirs)
install(TARGETS ${PROJECT} 
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
