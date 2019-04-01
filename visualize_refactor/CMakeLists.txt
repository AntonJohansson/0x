set(PROJECT visualize_refactor)
set(SOURCES
	tcp/socket.cpp
	tcp/socket_connection.cpp
	hex.cpp
	main.cpp
	)

set(CMAKE_MODULE_PATH "/usr/local/share/SFML/cmake/Modules")
find_package(SFML 2 COMPONENTS network graphics window system)

project(${PROJECT})
add_executable(${PROJECT} ${SOURCES})

target_compile_options(${PROJECT} PRIVATE
	-std=c++1z -g
	)

include_directories(${SFML_INCLUDE_DIR})

if(UNIX AND NOT APPLE)
	# unix
	target_link_libraries(${PROJECT}
		sfml-graphics sfml-window sfml-system pthread
		)
elseif(APPLE)
	# mac
	target_link_libraries(${PROJECT} ${SFML_LIBRARIES} pthread)
endif()

