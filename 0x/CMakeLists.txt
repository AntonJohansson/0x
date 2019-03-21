set(PROJECT 0x)
set(SOURCES
	tcp/socket_connection.cpp
	tcp/socket.cpp
	network/serialize_data.cpp
	game.cpp
	server.cpp
	hex.cpp
	main.cpp
	)

project(${PROJECT})
add_executable(${PROJECT} ${SOURCES})

target_compile_options(${PROJECT} PRIVATE
	-std=c++17 -g
	)

target_link_libraries(${PROJECT}
	pthread
	)
