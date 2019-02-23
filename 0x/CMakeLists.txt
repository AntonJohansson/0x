set(PROJECT 0x)
set(SOURCES
	server_connection.cpp
	socket.cpp
	main.cpp
	)

project(${PROJECT})
add_executable(${PROJECT} ${SOURCES})

target_compile_options(${PROJECT} PRIVATE
	-std=c++1z -g
	)
